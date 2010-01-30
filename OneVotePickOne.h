#ifndef ONE_VOTEPICKONE_H
#define ONE_VOTEPICKONE_H

#include "VotingSystem.h"

class OneVotePickOne : public VotingSystem {
public:
    OneVotePickOne() : VotingSystem( "One Vote" ) {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~OneVotePickOne();
};

#endif
