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
#include <sys/time.h>
#include <sys/resource.h>

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

// somewhere code externs this and needs it
volatile int goGently = 0;

inline double tv2d(const struct timeval& tv) {
  return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

int main( int argc, char** argv ) {
	VSConfig* systemList = NULL;
	VotingSystem** systems;
	systemList = VSConfig::defaultList();

	int nsys = 0;
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

        int numv = 500000;
        int numc = 10;
        VoterArray they;
        they.build(numv, numc);
        int* winners = new int[numc];
        printf("# %d choices, %d votes\n# name\tvotes/second\n", numc, numv);
        for ( int sys = 0; sys < nsys; sys++ ) {
          struct rusage start, end;
          getrusage(RUSAGE_SELF, &start);
          systems[sys]->runElection( winners, they );
          getrusage(RUSAGE_SELF, &end);
          double dt = tv2d(end.ru_utime) - tv2d(start.ru_utime);
          printf("%s\t%f\n", systems[sys]->name, numv/dt);
        }
	return 0;
}
