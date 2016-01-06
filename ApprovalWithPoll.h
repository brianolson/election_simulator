#ifndef APPROVALWITHPOLL_H
#define APPROVALWITHPOLL_H

#include "VotingSystem.h"

class ApprovalWithPoll : public VotingSystem {
public:
    ApprovalWithPoll() : VotingSystem( "Approval With Poll" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~ApprovalWithPoll();
};

#endif
