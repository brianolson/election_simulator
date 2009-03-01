#ifndef RANKED_VOTE_PICK_ONE_H
#define RANKED_VOTE_PICK_ONE_H

#include "VotingSystem.h"

class RankedVotePickOne : public VotingSystem {
public:
    int dontCountNegPref;
    RankedVotePickOne()
     : VotingSystem( "Borda" ), dontCountNegPref( 0 ) {};
    RankedVotePickOne( const char* n )
     : VotingSystem( n ), dontCountNegPref( 0 ) {};
    RankedVotePickOne( const char* n, int dcnp )
     : VotingSystem( n ), dontCountNegPref( dcnp ) {};
    virtual void runElection( int* winnerR, const VoterArray& they );
    virtual ~RankedVotePickOne();
};

#endif
