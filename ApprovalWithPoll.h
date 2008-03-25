#ifndef APPROVALWITHPOLL_H
#define APPROVALWITHPOLL_H

#include "VotingSystem.h"

class ApprovalWithPoll : public VotingSystem {
public:
    ApprovalWithPoll() : VotingSystem( "ApprovalWithPoll" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they );
    virtual ~ApprovalWithPoll();
};

#endif
