#ifndef INSTANTRUNOFFVOTEPICKONE_H
#define INSTANTRUNOFFVOTEPICKONE_H

#include "VotingSystem.h"

class TopNRunoff : public VotingSystem {
public:
    TopNRunoff()
     : VotingSystem("Top N Runoff") {};
	virtual void init( const char** envp );
    virtual void runElection( int* winnerR, const VoterArray& they );
	
protected:
	int finalRoundSize;
};

#endif

