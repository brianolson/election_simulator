#include "Voter.h"
#include "ApprovalNoInfo.h"
#include <stdlib.h>

#ifdef USE_OLD_RAND
/* only use this if your libc doesn't have the newer better random() */
static inline long random() {
    return rand();
}
#endif

/*
 Contributed by Raphfrk
 with optimizations and integration by Brian Olson
 */

void ApprovalNoInfo::runElection( int* winnerR, const VoterArray& they ) {
	int i,j;
    int* talley;
    int numc = they.numc;
    int numv = they.numv;
    
    // init things
    talley = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }

    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
		double thresh;
		thresh = 0.0;
        for ( j = 0; j < numc; j++) {
            thresh += they[i].getPref( j );
        }
        thresh = thresh/numc;
        for ( j = 0; j < numc; j++) {
            if ( they[i].getPref( j ) >= thresh ) {
				talley[j]++;
            }
        }
    }
    /* init winner array for sort later */
    for ( i = 0; i < numc; i++ ) {
	winnerR[i] = i;
    }
    {
	/* sort winner array based on tallies */
	int done;
	done = 0;
	while ( ! done ) {
	    done = 1;
	    for ( i = 0; i < numc - 1; i ++ ) {
		if ( talley[winnerR[i]] < talley[winnerR[i+1]] ) {
		    int t;
		    t = winnerR[i];
		    winnerR[i] = winnerR[i+1];
		    winnerR[i+1] = t;
		    done = 0;
		}
	    }
	}
    }
    {
	/* if tie, pick a random winner */
	int tied = 1;
	for ( i = 1; i < numc; i++ ) {
	    if ( talley[winnerR[i]] == talley[winnerR[0]] ) {
		tied++;
	    }
	}
	if ( tied > 1 ) {
	    int promote = random() % tied;
	    if ( promote != 0 ) {
		int t = winnerR[promote];
		winnerR[promote] = winnerR[0];
		winnerR[0] = t;
	    }
	}
    }
    delete [] talley;
}

VotingSystem* newApprovalNoInfo( const char* n ) {
	return new ApprovalNoInfo();
}
VSFactory* ApprovalNoInfo_f = new VSFactory( newApprovalNoInfo, "ApprovalNoInfo" );

ApprovalNoInfo::~ApprovalNoInfo() {
}
