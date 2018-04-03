#ifndef STARVOTE_H
#define STARVOTE_H

#include "VotingSystem.h"

class STARVote : public VotingSystem {
	public:
	STARVote() : VotingSystem("STAR") {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~STARVote();
};

#endif /* STARVOTE_H */
