#include "Voter.h"
#include "RankedVotePickOne.h"

/**
 * This procedure is also known as a "Borda count", 
 * where for N candidates, a highest-rank-vote is worth N points, a 2nd place vote
 * is worth N-1, and so on.
 
 * TO DO, modify for Nanson/Baldwin progrissive mode. (Instant Runoff Borda?)
 */
void RankedVotePickOne::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    unsigned int* talley;
    int winner;
    unsigned int* sortedIndecies;
    int numc = they.numc;
    int numv = they.numv;
    
    // init things
    talley = new unsigned int[numc];
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }
    
    sortedIndecies = new unsigned int[numc];
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        int j;
        const Voter* v;
        v = &(they[i]);
        sortedIndecies[0] = 0;
        // insertion sort
        for ( j = 1; j < numc; j++ ) {
            int k;
            k = j - 1;
            while ( v->getPref(sortedIndecies[k]) < v->getPref(j) ) {
                sortedIndecies[k+1] = sortedIndecies[k];
                k--;
                if ( k < 0 )
                    break;
            }
            sortedIndecies[k+1] = j;
        }
        for ( j = 0; j < numc; j++ ) {
            if ( dontCountNegPref == 1 && v->getPref(sortedIndecies[j]) < 0.0 )
                break;
            talley[sortedIndecies[j]] += numc - j + 1;
        }
    }
    // find winner
    {
        unsigned int m = talley[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m = talley[i];
                winner = i;
            }
        }
    }
    delete [] talley;
    delete [] sortedIndecies;
    if ( winnerR ) *winnerR = winner;
}


VotingSystem* newBorda( const char* n ) {
	return new RankedVotePickOne( n );
}
VSFactory* Borda_f = new VSFactory( newBorda, "Borda" );

RankedVotePickOne::~RankedVotePickOne() {
}
