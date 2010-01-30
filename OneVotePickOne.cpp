#include "Voter.h"
#include "OneVotePickOne.h"

void OneVotePickOne::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    int* talley;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
    
    // init things
    talley = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }
    
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        talley[they[i].getMax()]++;
    }
    // find winner
    {
        int m = talley[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m = talley[i];
                winner = i;
            }
        }
    }
    delete [] talley;
    if ( winnerR ) *winnerR = winner;
}

VotingSystem* newOneVotePickOne( const char* n ) {
	return new OneVotePickOne();
}
VSFactory* OneVotePickOne_f = new VSFactory( newOneVotePickOne, "OneVote" );

OneVotePickOne::~OneVotePickOne() {
}
