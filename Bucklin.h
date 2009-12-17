#ifndef BUCKLIN_H
#define BUCKLIN_H

#include "VotingSystem.h"

// http://en.wikipedia.org/wiki/Bucklin_voting
class Bucklin : public VotingSystem {
public:
	Bucklin() : VotingSystem( "Bucklin" ) {};
	//virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they );
	virtual ~Bucklin();
};

#endif
