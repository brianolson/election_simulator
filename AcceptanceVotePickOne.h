#ifndef ACCEPTANCEVOTEPICKONE_H
#define ACCEPTANCEVOTEPICKONE_H

#include "VotingSystem.h"

class AcceptanceVotePickOne : public VotingSystem {
public:
    AcceptanceVotePickOne() : VotingSystem( "Approval" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they );
};

#endif
