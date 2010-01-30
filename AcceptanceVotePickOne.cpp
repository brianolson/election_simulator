#include "Voter.h"
#include "AcceptanceVotePickOne.h"

void AcceptanceVotePickOne::runElection( int* winnerR, const VoterArray& they ) const {
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
        for ( int c = 0; c < numc; c++ ) {
            if ( they[i].getPref(c) > 0 ) {
                talley[c]++;
            }
        }
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

VotingSystem* newAcceptanceVotePickOne( const char* n ) {
	return new AcceptanceVotePickOne();
}
VSFactory* AcceptanceVotePickOne_f = new VSFactory( newAcceptanceVotePickOne, "Approval" );
