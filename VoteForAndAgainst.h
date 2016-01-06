#ifndef VOTEFORANDAGAINST_H
#define VOTEFORANDAGAINST_H

#include "VotingSystem.h"

class VoteForAndAgainst : public VotingSystem {
public:
    VoteForAndAgainst() : VotingSystem( "Vote For And Against" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~VoteForAndAgainst();
};

#endif
