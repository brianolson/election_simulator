#ifndef APPROVAL_NO_INFO_H
#define APPROVAL_NO_INFO_H

#include "VotingSystem.h"

class ApprovalNoInfo : public VotingSystem {
public:
    ApprovalNoInfo() : VotingSystem( "Approval No Info" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~ApprovalNoInfo();
};

#endif
