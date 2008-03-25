#ifndef INSTANTRUNOFFVOTEPICKONE_H
#define INSTANTRUNOFFVOTEPICKONE_H

#include "VotingSystem.h"

class InstantRunoffVotePickOne : public VotingSystem {
public:
    InstantRunoffVotePickOne()
     : VotingSystem("Instant Runoff Vote") {};
    virtual void runElection( int* winnerR, const VoterArray& they );
};

#endif

