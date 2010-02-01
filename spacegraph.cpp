#include "GaussianRandom.h"
#include "PlaneSim.h"
#include "PlaneSimDraw.h"
#include "VotingSystem.h"
#include "XYSource.h"

#include "Condorcet.h"
#include "IRNRP.h"
#include "STV.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 64
#endif

// This doesn't do anything but make sure these methods get linked in.
void* linker_tricking() {
	delete new STV();
	return new IRNRP();
}

volatile int goGently = 0;

void mysigint( int a ) {
	goGently = 1;
}

// TODO: put these into a spacegraph_util.h
void printEMList(void);
VotingSystem** getVotingSystemsFromMethodNames(const char* methodNames, int* length_OUT);
char* fileTemplate(const char* tpl, const char* methodName, const char* plane);

const char* usage =
"usage: spacegraph [-o main output png file name template]\n"
"\t[-tg test gauss png file name]\n"
"\t[-minx f][-miny f][-maxx f][-maxy f]\n"
"\t[-px i][-py i][-v voters][-n iter per pix][-Z sigma]\n"
"\t[-c \"candidateX Y\"][--list]\n"
"\t[--method electionmethod[,...]]\n"
"\t[--random random device][--planes plane file template]\n"
;

#ifndef MAX_METHOD_ENV
#define MAX_METHOD_ENV 200
#endif
char defaultFoutname[] = "tsg.png";

static bool maybeLongarg(const char* arg, const char* prefix, const char** optarg) {
	const char* a = arg;
	const char* b = prefix;
	while (*a == *b) {
		a++;
		b++;
		if (*a == '\0') {
			if (*b == '\0') {
				if (optarg != NULL) {
					*optarg = NULL;
				}
				return true;
			} else {
				return false;
			}
		} else if (*a == '=') {
			if (*b != '\0') {
				return false;
			}
			if (optarg == NULL) {
				return false;
			}
			*optarg = a + 1;
			return true;
		}
	}
	return false;
}

int main( int argc, char** argv ) {
	const char* votingSystemName = NULL;
	const char** methodEnv = new const char*[MAX_METHOD_ENV];
	int methodEnvCount = 0;
	PlaneSim sim;
	int i;
	char* foutname = defaultFoutname;
	char* testgauss = NULL;
	int nvoters = 1000;
	int nthreads = 1;
	const char* planesPrefix = NULL;
	const char* optarg;
	const char* randomDevice = NULL;

	for ( i = 1; i < argc; i++ ) {
		if ( ! strcmp( argv[i], "-o" ) ) {
			i++;
			foutname = argv[i];
		} else if ( ! strcmp( argv[i], "-tg" ) ) {
			i++;
			testgauss = argv[i];
		} else if ( ! strcmp( argv[i], "-minx" ) ) {
			i++;
			sim.minx = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-maxx" ) ) {
			i++;
			sim.maxx = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-miny" ) ) {
			i++;
			sim.miny = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-maxy" ) ) {
			i++;
			sim.maxy = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-px" ) ) {
			i++;
			sim.px = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-py" ) ) {
			i++;
			sim.py = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-v" ) ) {
			i++;
			nvoters = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-n" ) ) {
			i++;
			sim.electionsPerPixel = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-Z" ) ) {
			i++;
			sim.voterSigma = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-c" ) ) {
			i++;
			sim.addCandidateArg( argv[i] );
		} else if ( ! strcmp( argv[i], "--list" ) ) {
			printEMList();
			exit(0);
		} else if ( ! strcmp( argv[i], "--manhattan" ) ) {
			sim.manhattanDistance = true;
		} else if ( ! strcmp( argv[i], "--linearFalloff" ) ) {
			sim.linearFalloff = true;
		} else if (maybeLongarg(argv[i], "--method", &optarg)) {
			if (!optarg) { i++; optarg = argv[i]; }
			votingSystemName = optarg;
		} else if ( ! strcmp( argv[i], "--opt" ) ) {
			i++;
			methodEnv[methodEnvCount] = argv[i];
			methodEnvCount++;
		} else if ( ! strcmp( argv[i], "--threads" ) ) {
			i++;
			nthreads = strtol( argv[i], NULL, 10 );
		} else if (maybeLongarg(argv[i], "--seats", &optarg)) {
			if (!optarg) { i++; optarg = argv[i]; }
			sim.seats = strtol( optarg, NULL, 10 );
			char* seatsMethodArg = new char[20];  // FIXME: this leaks
			sprintf(seatsMethodArg, "seats=%d", sim.seats);
			methodEnv[methodEnvCount] = seatsMethodArg;
			methodEnvCount++;
		} else if (maybeLongarg(argv[i], "--planes", &optarg)) {
			if (!optarg) { i++; optarg = argv[i]; }
			planesPrefix = optarg;
		} else if (maybeLongarg(argv[i], "--combine", NULL)) {
			sim.doCombinatoricExplode = true;
		} else if (maybeLongarg(argv[i], "--random", &optarg)) {
			if (!optarg) { i++; optarg = argv[i]; }
			randomDevice = optarg;
		} else {
			fprintf( stderr, "bogus arg \"%s\"\n", argv[i] );
			fputs( usage, stderr );
			fputs( "Known election methods:\n", stderr );
			printEMList();
			exit(1);
		}
	}

	if ( sim.candcount == 0 && testgauss == NULL ) {
		fprintf( stderr, "error, no candidate positions specified\n");
		exit(1);
	}
	if ( randomDevice != NULL ) {
		sim.rootRandom = new FileDoubleRandom(randomDevice, 8*1024);
	} else {
		sim.rootRandom = new ClibDoubleRandom();
	}
	PlaneSimDraw* psd = new PlaneSimDraw(sim.px, sim.py, 3);
	if ( testgauss != NULL ) {
		sim.gRandom = new GaussianRandom(sim.rootRandom);
		sim.addCandidateArg("0,0");
		sim.build( 1 );
		psd->gaussTest( testgauss, nvoters, &sim );
		return 0;
	}
	int numVotingSystems = 1;
	VotingSystem** systems = NULL;
	if ( votingSystemName == NULL ) {
		systems = new VotingSystem*[numVotingSystems];
		systems[0] = new Condorcet();
	} else {
		systems = getVotingSystemsFromMethodNames(votingSystemName, &numVotingSystems);
	}
	if ( methodEnvCount > 0 ) {
		methodEnv[methodEnvCount] = NULL;
		for (int vi = 0; vi < numVotingSystems; ++vi) {
			systems[vi]->init( methodEnv );
		}
	}
	sim.build( nvoters );
	{
	    char cfgstr[512];
	    sim.configStr( cfgstr, sizeof(cfgstr) );
	    fputs( cfgstr, stdout );
	    fputs( "\n", stdout );
	}
	sim.setVotingSystems(systems, numVotingSystems);
	XYSource* source = new XYSource(sim.px, sim.py);
	if ( nthreads <= 1 ) {
		sim.gRandom = new GaussianRandom(sim.rootRandom);
		sim.runXYSource( source );
	} else {
		sim.rootRandom = new LockingDoubleRandomWrapper(sim.rootRandom);
		sim.gRandom = new GaussianRandom(
			new BufferDoubleRandomWrapper(sim.rootRandom, 512, true));
		PlaneSim* sims = new PlaneSim[nthreads-1];
		PlaneSimThread* threads = new PlaneSimThread[nthreads];
		int i;
		for (i = 0; i < nthreads-1; ++i) {
			sims[i].coBuild(sim);
			threads[i].sim = &(sims[i]);
			threads[i].source = source;
		}
		threads[i].sim = &sim;
		threads[i].source = source;
		for (i = 0; i < nthreads; ++i) {
			pthread_create(&(threads[i].thread), NULL, runPlaneSimThread, &(threads[i]));
		}
		for (i = 0; i < nthreads; ++i) {
			pthread_join(threads[i].thread, NULL);
		}
	}
	for (int vi = 0; vi < numVotingSystems; ++vi) {
		char* pfilename;
		if (planesPrefix) {
			static char planeStr[20];
			for (int plane = 0; plane < sim.they.numc; ++plane) {
				snprintf(planeStr, sizeof(planeStr), "%d", plane);
				pfilename = fileTemplate(planesPrefix, sim.systems[vi]->name, planeStr);
				psd->writePlanePNG(pfilename, plane, &sim, sim.accum[vi]);
				free(pfilename);
			}
			pfilename = fileTemplate(planesPrefix, sim.systems[vi]->name, "_sum.png");
			psd->writeSumPNG(pfilename, &sim, sim.accum[vi]);
			free(pfilename);
		}
		pfilename = fileTemplate(foutname, sim.systems[vi]->name, NULL);
		psd->writePNG( pfilename, &sim, sim.accum[vi] );
		free(pfilename);
	}
}
