#include "PlaneSim.h"
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

static void printEMList(void) {
	VSFactory* cf;
	cf = VSFactory::root;
	while ( cf != NULL ) {
		printf("%s\n", cf->name );
		cf = cf->next;
	}
}

const char* usage =
"usage: spacegraph [-o foo.png][-tg][-minx f][-miny f][-maxx f][-maxy f]\n"
"\t[-px i][-py i][-v voters][-n iter per pix][-Z sigma]\n"
"\t[-c \"candidateX Y\"][--list][--method electionmethod]\n"
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
	// trivial main
	const char* votingSystemName = NULL;
	VotingSystem* vs = NULL;
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
	if ( testgauss != NULL ) {
		sim.addCandidateArg("0,0");
		sim.build( 1 );
		sim.gaussTest( testgauss, nvoters );
		return 0;
	}
	if ( votingSystemName == NULL ) {
		vs = new Condorcet();
	} else {
		const VSFactory* cf;
		cf = VSFactory::byName(votingSystemName);
		if ( cf == NULL ) {
			fprintf(stderr,"no such election method \"%s\", have:\n", votingSystemName );
			printEMList();
			exit(1);
		}
		vs = cf->make();
	}
	if ( methodEnvCount > 0 ) {
		methodEnv[methodEnvCount] = NULL;
		vs->init( methodEnv );
	}
	sim.build( nvoters );
	{
	    char cfgstr[512];
	    sim.configStr( cfgstr, sizeof(cfgstr) );
	    fputs( cfgstr, stdout );
	    fputs( "\n", stdout );
	}
	if ( nthreads <= 1 ) {
		sim.run( vs );
	} else {
		PlaneSim* sims = new PlaneSim[nthreads-1];
		PlaneSimThread* threads = new PlaneSimThread[nthreads];
		XYSource* source = new XYSource(sim.px, sim.py);
		int i;
		for (i = 0; i < nthreads-1; ++i) {
			sims[i].coBuild(sim);
			threads[i].sim = &(sims[i]);
			threads[i].vs = vs;
			threads[i].source = source;
		}
		threads[i].sim = &sim;
		threads[i].vs = vs;
		threads[i].source = source;
		for (i = 0; i < nthreads; ++i) {
			pthread_create(&(threads[i].thread), NULL, runPlaneSimThread, &(threads[i]));
		}
		for (i = 0; i < nthreads; ++i) {
			pthread_join(threads[i].thread, NULL);
		}
	}
	if (planesPrefix) {
		int prefixlen = strlen(planesPrefix);
		char* pfilename = new char[prefixlen + 20];
		strcpy(pfilename, planesPrefix);
		for (int plane = 0; plane < sim.they.numc; ++plane) {
			sprintf(pfilename + prefixlen, "%d.png", plane);
			sim.writePlanePNG(pfilename, plane);
		}
		strcpy(pfilename + prefixlen, "_sum.png");
		sim.writeSumPNG(pfilename);
	}
	sim.writePNG( foutname );
}
