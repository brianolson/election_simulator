#ifndef STARVOTE_H
#define STARVOTE_H

#include "VotingSystem.h"

class STARVote : public VotingSystem {
	public:
    STARVote() : VotingSystem("STAR"), quantization(5) {};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
    virtual ~STARVote();

    private:
    int quantization;
};

#endif /* STARVOTE_H */
