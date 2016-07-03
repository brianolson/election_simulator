#include "GaussianRandom.h"
#include "VotingSystem.h"
#include "PlaneSim.h"
#include "PlaneSimDraw.h"
#include "spacegraph_util.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "trial.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
using google::protobuf::io::FileInputStream;

void printEMList(void) {
	VSFactory* cf;
	cf = VSFactory::root;
	while ( cf != NULL ) {
		printf("%s\n", cf->name );
		cf = cf->next;
	}
}

// return array of VotingSystem*, length in length_OUT param
VotingSystem** getVotingSystemsFromMethodNames(const char* methodNames, int* length_OUT) {
	int numVotingSystems = 1;
	VotingSystem** systems = NULL;
	const VSFactory* cf;
	char* vsnameScratch = strdup(methodNames);
	char* vsstart = vsnameScratch;
	char* pos = vsnameScratch;
	while (*pos != '\0') {
		if (*pos == ',') {
			numVotingSystems++;
		}
		pos++;
	}
	pos = vsnameScratch;
	systems = new VotingSystem*[numVotingSystems];
	int si = 0;
	while (*pos != '\0') {
		if (*pos == ',') {
			*pos = '\0';
			cf = VSFactory::byName(vsstart);
			if ( cf == NULL ) {
				fprintf(stderr,"no such election method \"%s\", have:\n", vsstart );
				printEMList();
				exit(1);
			}
			systems[si] = cf->make();
			si++;
			vsstart = pos + 1;
		}
		pos++;
	}
	cf = VSFactory::byName(vsstart);
	if ( cf == NULL ) {
		fprintf(stderr,"no such election method \"%s\", have:\n", vsstart );
		printEMList();
		exit(1);
	}
	systems[si] = cf->make();
	si++;
	assert(si == numVotingSystems);
	free(vsnameScratch);
	*length_OUT = numVotingSystems;
	return systems;
}

#if HAVE_PROTOBUF
int readConfigFile(const char** configOutP, PlaneSim& sim, int* nvotersP, int* numVotingSystemsP, VotingSystem*** systemsP) {
	const char* configOut = *configOutP;
	int nvoters = *nvotersP;
	int numVotingSystems = *numVotingSystemsP;
	VotingSystem** systems = *systemsP;
	// If it already exists, read it and check that it is compatible with command line, otherwise write the file.
	Configuration config;
	int fd = open(configOut, O_RDONLY);
	if (fd >= 0) {
		FileInputStream* fis = new FileInputStream(fd);
		assert(fis);
		bool ok = config.ParseFromZeroCopyStream(fis);
		delete fis;
		close(fd);
		if (ok) {
			// Check that loaded config is ok.
			if (config.voters() != nvoters) {
				if (nvoters == nvoters_default) {
					nvoters = config.voters();
				} else {
					fprintf(stderr, "command line number of voters %d incompatible with value %d from config file %s\n", nvoters, config.voters(), configOut); exit(1); return 1;
				}
			}
			if (config.error() != 0.0) {
				fprintf(stderr, "don't know how to handle config file %s error value of %f\n", configOut, config.error()); exit(1); return 1;
			}
			if (config.dimensions() != 2) {
				fprintf(stderr, "don't know how to handle config file %s dimensions value of %d\n", configOut, config.dimensions()); exit(1); return 1;
			}
			if (config.voter_model() != Configuration::CANDIDATE_COORDS) {
				fprintf(stderr, "don't know how to handle config file %s voter_model value of %d\n", configOut, config.voter_model()); exit(1); return 1;
			}
			if (config.seats() != sim.seats) {
				if (sim.seats == 1) {
					sim.seats = config.seats();
				} else {
					fprintf(stderr, "command line number of seats %d incompatible with value %d from config file %s\n", sim.seats, config.seats(), configOut); exit(1); return 1;
				}
			}
			if (sim.candcount == 0) {
				for (int c = 0; c < config.candidate_coords_size()/2; ++c) {
					sim.addCandidateArg(config.candidate_coords(c*2 + 0), config.candidate_coords(c*2 + 1));
				}
			} else {
				if (config.candidate_coords_size()/2 != sim.candcount) {
					fprintf(stderr, "incompatible number of choices between config file (%d) and command (%d)\n", config.candidate_coords_size()/2, sim.candcount); exit(1); return 1;
				}
				PlaneSim::candidatearg* cand = sim.candroot;
				for (int c = 0; c < config.candidate_coords_size()/2; ++c) {
					if ((config.candidate_coords(c*2 + 0) != cand->x) ||
						(config.candidate_coords(c*2 + 1) != cand->y)) {
						fprintf(stderr, "coord %d doesn't match: %s (%f,%f) cmd (%f,%f)\n", c, configOut, config.candidate_coords(c*2 + 0), config.candidate_coords(c*2 + 1), cand->x, cand->y); exit(1); return 1;
					}
					cand = cand->next;
				}
			}
#define CHECK_CONFIG_VALUE(config_name, var, default, name) if (config.config_name() != var) { \
	if (var == default) { \
		var = config.config_name(); \
	} else { \
		fprintf(stderr, "command line "name" %f incompatible with value %f from config file %s\n", var, config.config_name(), configOut); exit(1); return 1; \
	} \
}
			CHECK_CONFIG_VALUE(voter_sigma, sim.voterSigma, 1.0, "voter sigma");
			CHECK_CONFIG_VALUE(minx, sim.minx, -1.0, "minx");
			CHECK_CONFIG_VALUE(maxx, sim.maxx, 1.0, "maxx");
			CHECK_CONFIG_VALUE(miny, sim.miny, -1.0, "miny");
			CHECK_CONFIG_VALUE(maxy, sim.maxy, 1.0, "maxy");
			if (systems == NULL) {
				numVotingSystems = config.system_names_size();
				systems = new VotingSystem*[numVotingSystems];
				for (int vi = 0; vi < numVotingSystems; ++vi) {
					const VSFactory* cf = VSFactory::byName(config.system_names(vi).c_str());
					if (cf == NULL) {
						fprintf(stderr, "not compiled with VSFactory for election method named \"%s\" from config file %s\n", config.system_names(vi).c_str(), configOut); exit(1); return 1;
					}
					systems[vi] = cf->make();
					assert(systems[vi] != NULL);
				}
			} else {
				for (int vi = 0; vi < numVotingSystems; ++vi) {
					if (strcmp(config.system_names(vi).c_str(), systems[vi]->name)) {
						fprintf(stderr, "system[%d] %s incompatible with value %s from config file %s\n", vi, systems[vi]->name, config.system_names(vi).c_str(), configOut); exit(1); return 1;
					}
				}
			}
			
#if 0
			if (config.choices() != sim.they.numc) {
				fprintf(stderr, "this run has %d choices but config file %s has %d\n", sim.they.numc, configOut, config.choices()); exit(1); return 1;
			}
#endif
			// don't write it out below
			*configOutP = NULL;
			*nvotersP = nvoters;
			*numVotingSystemsP = numVotingSystems;
			*systemsP = systems;
		} else {
			fprintf(stderr, "stored config in %s did not load, may be corrupted. exiting.\n", configOut);
			exit(1);
			return 1;
		}
	}
	return 0;
}
#endif

void testGauss(const char* path, PlaneSim* sim, int nvoters) {
	sim->gRandom = new GaussianRandom(sim->rootRandom);
	sim->addCandidateArg("0,0");
	sim->build( 1 );
	char* argtext = new char[1024];
	sim->configStr(argtext, 1024);
	PlaneSimDraw* drawGauss = new PlaneSimDraw(sim->px, sim->py, 3);
	drawGauss->gaussTest( path, nvoters, sim, argtext );
	delete drawGauss;
	delete [] argtext;
}
