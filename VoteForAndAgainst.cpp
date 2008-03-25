#include "Voter.h"
#include "VoteForAndAgainst.h"

/*
 Contributed by Kevin Venzke
 with optimizations and integration by Brian Olson
 */

void VoteForAndAgainst::runElection( int* winnerR, const VoterArray& they ) {
    int i;
    int* fptally;
    int* lptally;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
	
    // init things
    fptally = new int[numc];
    lptally = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        fptally[i] = 0;
        lptally[i] = 0;
    }
	
    // count first and last prefs for each candidate
    for ( i = 0; i < numv; i++ ) {
        fptally[they[i].getMax()]++;
        lptally[they[i].getMin()]++;
    }
	
    // find winner
    {
        int m = 0;
        winner = 0;
        for ( i = 0; i < numc; i++ ) {
            if ( fptally[i] >= m && lptally[i]*2 <= numv ) {
                m = fptally[i];
                winner = i;
            }
        }
    }
    delete [] fptally;
    delete [] lptally;
    if ( winnerR ) *winnerR = winner;
}

VotingSystem* newVoteForAndAgainst( const char* n ) {
	return new VoteForAndAgainst();
}
VSFactory* VoteForAndAgainst_f = new VSFactory( newVoteForAndAgainst, "VoteForAndAgainst" );

VoteForAndAgainst::~VoteForAndAgainst() {
}
