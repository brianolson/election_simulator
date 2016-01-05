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
#include "STV.h"
#include "IRNRP.h"
#include "RandomElection.h"

#include "ResultFile.h"
#include "ResultLog.h"
#include "WorkQueue.h"
#include "workQThread.h"

#if HAVE_PROTOBUF
#include "ProtoResultLog.h"
#include <fcntl.h>
#else
class ProtoResultLog;
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

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 64
#endif

static char voter_main_usage[] = 
"-F dbResultFile\n"
"--textout textResultFile\n"
"-Mef methodDescFile\n"
"-M methodArg\n"
"-e methodName -- consumes previous -M args initting this method\n"
"--list        -- show known methods and exit\n"
"--vsteps      -- space separated list of number-of-voters\n"
"--csteps      -- space separated list of number-of-choices\n"
"--esteps      -- space separated list of error rates\n"
"--CSsteps     -- space separated list of choices,seats pairs\n"
"-n iter       -- iterations to run\n"
"--threads n\n"
"\n"
"VoterSim::init options:\n"
"-v n          -- number of voters\n"
"-c n          -- number of choices\n"
"-e n          -- error rate\n"
"-n iter       -- iterations to run\n"
"-N iter lim   -- run up to iter lim if not there already\n"
"-P            -- print voters every run\n"
"-r            -- print results every run\n"
"-R            -- result dump\n"
"-H filename   -- result dump HTML\n"
"-h filename   -- result dump HTML alt format\n"
"-D filename   -- dump voters, binary\n"
"-d filename   -- dump voters, text\n"
"-L filename   -- load voters, binary\n"
"-l filename   -- load voters, text\n"
"-q            -- quiet\n"
"-s            -- enable strategies\n"
"-S int        -- summary print style\n"
"--nflat       -- voters and choices exist in N demensional uniformly random space\n"
"--ngauss      -- voters and choices exist in N demensional gaussian random space\n"
"--dimensions N\n"
"--independentprefs  -- voters have independent uniform -1..1 preferences for each choice\n"
;

int main( int argc, char** argv ) {
    Steps* stepq = NULL;
    VoterSim* s;
    workQThread** wqts;
    ResultFile* drf = NULL;
    NameBlock nb;
    int j = 0, i;
    int nsys;
    int numThreads = 1;
    int hang = 0;
	const char** methodArgs = (const char**)malloc( sizeof(char*) * MAX_METHOD_ARGS );
	int methodArgc = 0;
	VSConfig* systemList = NULL;
	VotingSystem** systems;
	
	ResultLog* rlog = NULL;

	assert(methodArgs);
    srandom(time(NULL));
	
    for ( i = 0; i < argc; i++ ) {
		if ( j != i ) {
			argv[j] = argv[i];
		}
		if ( ! strcmp( argv[i], "--textout" ) ) {
			i++;
			drf = TextDumpResultFile::open(argv[i]);
			if (drf == NULL) {
				fprintf(stderr, "error: could not process \"--textout %s\"\n", argv[i]);
				exit(1);
			}
#if HAVE_PROTOBUF
		} else if ( ! strcmp( argv[i], "--rlog" ) ) {
			i++;
			rlog = ProtoResultLog::openForAppend(argv[i]);
			if (rlog == NULL) {
				perror(argv[i]);
				exit(1);
			}
#endif
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
		} else if ( ! strcmp( argv[i], "--CSsteps" ) ) {
			i++;
			if ( stepq == NULL ) stepq = new Steps;
			stepq->parseCandidatesSeats( argv[i] );
		} else if ( (! strcmp( argv[i], "-n" )) && (stepq != NULL) ) {
			i++;j++;
			argv[j] = argv[i];
			stepq->n = atoi( argv[i] );
			j++;
		} else if ( ! strcmp( argv[i], "--threads" ) ) {
			i++;
			numThreads = atoi( argv[i] );
		} else if ( ! strcmp( argv[i], "--help" ) ) {
			fputs(voter_main_usage, stderr);
			exit(0);
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
	
    if (drf == NULL) {
#if 0
      fprintf(stderr, "error, no result output configured\n");
      exit(1);
#endif
    } else {
		drf->useNames( &nb );
	}
	if (rlog != NULL) {
		printf("main useNames\n");
		bool ok = rlog->useNames( &nb );
		assert(ok);
		if (!ok) {
			exit(1);
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
		int err = s[i].init( argc, argv );
		if (err < 0) {
			exit(1);
		}
		s[i].rlog = rlog;
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
	if (rlog != NULL) {
		for ( int i = 0; i < numThreads; ++i ) {
			s[i].rlog = NULL;
		}
		delete rlog;
	}
    delete [] s;
    if ( hang ) {
		goGently = 0;
		while ( ! goGently ) {}
    }
#if 1
	fprintf(stderr, "%d Condorcet elections run, %d without cycle (%f%% have cycles)\n",
			Condorcet::electionsRun, Condorcet::simpleElections,
			100.0 - ((Condorcet::simpleElections * 100.0) / Condorcet::electionsRun));
#endif
    return 0;
}
