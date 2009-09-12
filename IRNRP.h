#ifndef IRNRP_H
#define IRNRP_H

#include "VotingSystem.h"

class MaybeDebugLog;

class IRNRP : public VotingSystem {
public:
	IRNRP()
		: VotingSystem( "IRNRP" ), seats(1),
		debug(NULL) {};
	virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they );
	virtual ~IRNRP();
	
private:
	int seats;
	MaybeDebugLog* debug;
};

#endif
