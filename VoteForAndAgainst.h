#ifndef VOTEFORANDAGAINST_H
#define VOTEFORANDAGAINST_H

#include "VotingSystem.h"

class VoteForAndAgainst : public VotingSystem {
public:
    VoteForAndAgainst() : VotingSystem( "VoteForAndAgainst" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they );
    virtual ~VoteForAndAgainst();
};

#endif
