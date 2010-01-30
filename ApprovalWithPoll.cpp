#include "Voter.h"
#include "ApprovalWithPoll.h"

/*
 Contributed by Raphfrk
 with optimizations and integration by Brian Olson
 */

void ApprovalWithPoll::runElection( int* winnerR, const VoterArray& they ) const {
	int i,j;
    int* talley;
    int winner = -1;
	int second_place = -1;
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
            if ( they[i].getPref(j) >= thresh ) {
				talley[j]++;
            }
        }
    }
    // find winner + second place
    {
        int m = -1;
        int m2 = -2; // second place
        for ( i = 0; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m2 = m;
                m = talley[i];
                second_place=winner;
                winner = i;
            } else if ( talley[i] > m2 ) {
                m2 = talley[i];
                second_place = i;
            }
        }
    }
	
	// Above assumed to be 'poll'
	
    // init things
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }
	
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
		double thresh = ( they[i].getPref(winner) + they[i].getPref(second_place) ) / 2.0 ;
        for ( j = 0; j < numc; j++) {
            if ( they[i].getPref(j) >= thresh ) {
				talley[j]++;
            }
        }
    }
    // find winner + second place
    {
        int m = -1;
        int m2 = -2; // second place
        for ( i = 0; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m2 = m;
                m = talley[i];
                second_place=winner;
                winner = i;
            } else if ( talley[i] > m2 ) {
                m2 = talley[i];
                second_place = i;
            }
        }
    }
	
    delete [] talley;
    if ( winnerR ) {
		*winnerR = winner;
		*(winnerR+1) = second_place;
	}
}

VotingSystem* newApprovalWithPoll( const char* n ) {
	return new ApprovalWithPoll();
}
VSFactory* ApprovalWithPoll_f = new VSFactory( newApprovalWithPoll, "ApprovalWithPoll" );

ApprovalWithPoll::~ApprovalWithPoll() {
}
