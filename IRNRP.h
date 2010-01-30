#ifndef IRNRP_H
#define IRNRP_H

#include "VotingSystem.h"

class MaybeDebugLog;

class IRNRP : public VotingSystem {
public:
	IRNRP()
		: VotingSystem( "IRNRP" ), seatsDefault(1),
		  debug(NULL), l2norm(false) {};
	virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they ) const;
	virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	virtual ~IRNRP();
	
private:
	int seatsDefault;
	MaybeDebugLog* debug;
	bool l2norm;
};

#endif
