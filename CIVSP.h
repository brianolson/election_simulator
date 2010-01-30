#ifndef CIVSP_H
#define CIVSP_H

#include "VotingSystem.h"

class CIVSP : public VotingSystem {
public:
	CIVSP() : VotingSystem( "CIVSP" ) {};
	//virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they ) const;
	virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	virtual ~CIVSP();
};

#endif
