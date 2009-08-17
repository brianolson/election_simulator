#ifndef IRNRP_H
#define IRNRP_H

#include "VotingSystem.h"

class IRNRP : public VotingSystem {
public:
	IRNRP() : VotingSystem( "IRNRP" ), seats(1) {};
	virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they );
	virtual ~IRNRP();
	
private:
	int seats;
};

#endif
