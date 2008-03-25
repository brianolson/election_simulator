#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "Voter.h"
#include "VoterSim.h"
#include "VotingSystem.h"

#include "OneVotePickOne.h"
#include "RankedVotePickOne.h"
#include "AcceptanceVotePickOne.h"
#include "FuzzyVotePickOne.h"
#include "InstantRunoffVotePickOne.h"
#include "Condorcet.h"
#include "IRNR.h"
#include "RandomElection.h"

#include "ResultFile.h"
#include "DBResultFile.h"
#include "ThreadSafeDBRF.h"
#include "WorkQueue.h"
#include "NNSVSim.h"

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 16
#endif

volatile int goGently = 0;

int main( int argc, char** argv ) {
	NNSVSim* s;
    int nsys;
	int i, j = 0;
	int numThreads = 1;
	const char** methodArgs = (const char**)malloc( sizeof(char*) * MAX_METHOD_ARGS );
	int methodArgc = 0;
	VSConfig* systemList = VSConfig::newVSConfig( "Rated", NULL );
	VotingSystem** systems;
	int ngens = 1000;
	int gtests = 100;
	double gkeep = 0.5;
	int popsize = 10;
	int numc = 4;

	srandom(time(NULL));

	for ( i = 0; i < argc; i++ ) {
		if ( j != i ) {
			argv[j] = argv[i];
		}
		if ( ! strcmp( argv[i], "-Mef" ) ) {
			i++;
			systemList = systemsFromDescFile( argv[i], methodArgs, MAX_METHOD_ARGS, systemList );
			methodArgc = 0;
		} else if ( ! strcmp( argv[i], "-M" ) ) {
			i++;
			if ( methodArgc < MAX_METHOD_ARGS ) {
				methodArgs[methodArgc] = argv[i];
				methodArgc++;
			} else {
				fprintf( stderr, "arg \"%s\" Is beyond limit of %d method args\n", argv[i], MAX_METHOD_ARGS );
			}
		} else if ( ! strcmp( argv[i], "-e" ) ) {
			// enable a system, commit its args
			i++;
			if ( methodArgc > 0 ) {
				const char** marg = new const char*[methodArgc+1];
				for ( int j = 0; j < methodArgc; j++ ) {
					marg[j] = methodArgs[j];
				}
				marg[methodArgc] = NULL;
				systemList = VSConfig::newVSConfig( argv[i], marg, systemList );
			} else {
				systemList = VSConfig::newVSConfig( argv[i], NULL, systemList );
			}
			methodArgc = 0;
		} else if ( ! strcmp( argv[i], "--list" ) ) {
			VSFactory* cf;
			cf = VSFactory::root;
			while ( cf != NULL ) {
				printf("%s\n", cf->name );
				cf = cf->next;
			}
			exit(0);
		} else if ( ! strcmp( argv[i], "-n" ) ) {
			i++;
			ngens = atoi( argv[i] );
		} else {
			j++;
		}
	}
    argv[j] = argv[i];
    argc = j;
	
	nsys = 0;
	{
		VSConfig* curSys;
		int spos = 0;
		if ( systemList == NULL ) {
			systemList = VSConfig::defaultList();
		}
		systemList = systemList->reverseList();
		curSys = systemList;
		while ( curSys ) {
			curSys = curSys->next;
			nsys++;
		}
		systems = (VotingSystem**)malloc( sizeof(VotingSystem*) * nsys );
		curSys = systemList;
		while ( curSys ) {
			systems[spos] = curSys->getVS();
			spos++;
			curSys = curSys->next;
		}
	}

    s = new NNSVSim[numThreads];
    for ( int i = 0; i < numThreads; i++ ) {
		s[i].systems = systems;
		s[i].nsys = nsys;
#if 0
		for ( int ai = 0; ai < argc; ai++ ) {
			printf("argv[%d] \"%s\"\n", ai, argv[ai] );
		}
#endif
//		s[i].init( argc, argv );
    }
	int mostfiti;
	float maxfit;
	double avgfit;
	s[0].initPop( popsize, numc );
	s[0].testg( gtests );
	for ( int gen = 0; gen < ngens; gen++ ) {
		//	s[0].print(stdout);
		s[0].nextg( gkeep );
		//	s[0].print(stdout);
		s[0].testg( gtests );
		if ( (gen % 100) == 0 ) {
			mostfiti = 0;
			maxfit = -1.9;
			avgfit = 0.0;
			for ( int i = 0; i < s[0].numv; i++ ) {
				if ( s[0].they[i].strategySuccess > maxfit ) {
					maxfit = s[0].they[i].strategySuccess;
					avgfit += s[0].they[i].strategySuccess;
					mostfiti = i;
				}
			}
			avgfit /= s[0].numv;
			printf("%d\t%g\t%g\n", gen, avgfit, maxfit );
		}
	}
	mostfiti = 0;
	maxfit = -1.9;
	for ( int i = 0; i < s[0].numv; i++ ) {
		if ( s[0].they[i].strategySuccess > maxfit ) {
			maxfit = s[0].they[i].strategySuccess;
			mostfiti = i;
		}
	}
	//s[0].print(stdout);
	printf("max @ %d:\n", mostfiti );
	s[0].they[mostfiti].print(stdout);
	printf("\n");
	{
		static float testPrefs[][4] = {
			{ 1.0, 0.5, -0.5, -1.0 }
		};
		static double nopoll[4] = { 0, 0, 0, 0 };
		for ( int i = 0; i < 1; i++ ) {
			s[0].they[mostfiti].setRealPref( testPrefs[i] );
			s[0].they[mostfiti].calcStrategicPref( nopoll );
			s[0].they[mostfiti].printRealPref( stdout );
			printf("\n");
			s[0].they[mostfiti].printStrategicPref( stdout );
			printf("\n");
		}
	}
}
