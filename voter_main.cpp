#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

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
#include "workQThread.h"

volatile int goGently = 0;

void mysigint( int a ) {
    goGently = 1;
}

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 64
#endif

/*
voter_main options:
-F dbResultFile
-Mef methodDescFile
-M methodArg
-e methodName -- consumes previous -M args initting this method
--list        -- show known methods and exit
--vsteps      -- comma separated list of number-of-voters
--csteps      -- comma separated list of number-of-choices
--esteps      -- comma separated list of error rates
-n iter       -- iterations to run
--threads n

VoterSim::init options:
-v n          -- number of voters
-c n          -- number of choices
-e n          -- error rate
-n iter       -- iterations to run
-N iter lim   -- run up to iter lim if not there already
-P            -- print voters every run
-r            -- print results every run
-R            -- result dump
-H filename   -- result dump HTML
-h filename   -- result dump HTML alt format
-D filename   -- dump voters, binary
-d filename   -- dump voters, text
-L filename   -- load voters, binary
-l filename   -- load voters, text
-q            -- quiet
-s            -- enable strategies
-S int        -- summary print style

*/

int main( int argc, char** argv ) {
    Steps* stepq = NULL;
    VoterSim* s;
    workQThread** wqts;
    DBResultFile* drf = NULL;
    char* drfname = NULL;
    NameBlock nb;
    int j = 0, i;
    int nsys;
    int numThreads = 1;
    int hang = 0;
	const char** methodArgs = (const char**)malloc( sizeof(char*) * MAX_METHOD_ARGS );
	int methodArgc = 0;
	VSConfig* systemList = NULL;
	VotingSystem** systems;

	assert(methodArgs);
    srandom(time(NULL));
	
    for ( i = 0; i < argc; i++ ) {
		if ( j != i ) {
			argv[j] = argv[i];
		}
		if ( ! strcmp( argv[i], "-F" ) ) {
			i++;
			drfname = argv[i];
		} else if ( ! strcmp( argv[i], "-Mef" ) ) {
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
		} else if ( ! strcmp( argv[i], "--vsteps" ) ) {
			i++;
			if ( stepq == NULL ) stepq = new Steps;
			stepq->parseV( argv[i] );
		} else if ( ! strcmp( argv[i], "--csteps" ) ) {
			i++;
			if ( stepq == NULL ) stepq = new Steps;
			stepq->parseC( argv[i] );
		} else if ( ! strcmp( argv[i], "--esteps" ) ) {
			i++;
			if ( stepq == NULL ) stepq = new Steps;
			stepq->parseE( argv[i] );
		} else if ( (! strcmp( argv[i], "-n" )) && (stepq != NULL) ) {
			i++;j++;
			argv[j] = argv[i];
			stepq->n = atoi( argv[i] );
			j++;
		} else if ( ! strcmp( argv[i], "--threads" ) ) {
			i++;
			numThreads = atoi( argv[i] );
		} else if ( ! strcmp( argv[i], "--hang" ) ) {
			hang = 1;
		} else {
			j++;
		}
    }
    argv[j] = argv[i];
    argc = j;
    free( methodArgs );
    methodArgs = NULL;
	
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
		assert(systems);
		curSys = systemList;
		while ( curSys ) {
			systems[spos] = curSys->getVS();
			spos++;
			curSys = curSys->next;
		}
	}
	votingSystemArrayToNameBlock( &nb, systems, nsys );
	
    if ( drfname != NULL ) {
		printf("opening result db \"%s\"...", drfname );
		if ( numThreads == 1 ) {
			drf = DBResultFile::open( drfname );
		} else {
			drf = ThreadSafeDBRF::open( drfname );
		}
		if ( drf ) {
			printf("success\n");
			drf->useNames( &nb );
		} else {
			printf("failed\n");
		}
    }
    
    signal( SIGINT, mysigint );
    
    s = new VoterSim[numThreads];
    for ( int i = 0; i < numThreads; i++ ) {
		s[i].systems = systems;
		s[i].nsys = nsys;
#if 0
		for ( int ai = 0; ai < argc; ai++ ) {
			printf("argv[%d] \"%s\"\n", ai, argv[ai] );
		}
#endif
		s[i].init( argc, argv );
    }
    if ( stepq == NULL ) {
		s[0].run( drf, nb );
    } else if ( numThreads == 1 ) {
		printf("running work set\n");
		s[0].runFromWorkQueue( drf, nb, stepq );
		printf("done\n");
    } else {
		WorkSource* q;
		printf("running %d threads...", numThreads);
		q = new ThreadSafeWorkSource( stepq );
		wqts = new workQThread*[numThreads];
		for ( int i = 0; i < numThreads; i++ ) {
			wqts[i] = new workQThread( s + i, drf, &nb, q );
		}
		printf("running\n");
		for ( int i = 0; i < numThreads; i++ ) {
			pthread_join( wqts[i]->thread, NULL );
			printf("thread %d done\n", i );
		}
		printf("all threads done\n");
		for ( int i = 0; i < numThreads; i++ ) {
			delete wqts[i];
		}
		delete [] wqts;
		delete q;
    }
    if ( hang ) {
		goGently = 0;
		while ( ! goGently ) {}
    }
    if ( drf ) {
		drf->flush();
		drf->close();
    }
    delete [] s;
    if ( hang ) {
		goGently = 0;
		while ( ! goGently ) {}
    }
    return 0;
}
