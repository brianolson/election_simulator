#ifndef CIVSP_H
#define CIVSP_H

// Based on Andrew Myers adaptation of Condorcet's method to proportional elections
// http://www.cs.cornell.edu/w8/~andru/civs/proportional.html

#include "VotingSystem.h"
#include "Condorcet.h"

class CIVSP : public Condorcet {
public:
	CIVSP() : Condorcet/*VotingSystem*/( "CIVSP" ) {};
	//virtual void init( const char** envp );
	// inherit Condorcet single winner implementation
	//virtual void runElection( int* winnerR, const VoterArray& they ) const;
	virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	virtual ~CIVSP();
};

#endif
