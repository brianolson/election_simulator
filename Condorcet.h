#ifndef CONDORCET_H
#define CONDORCET_H

#include "VotingSystem.h"

class Condorcet : public VotingSystem {
public:
    Condorcet()
     : VotingSystem( "Condorcet" ){};
    Condorcet( const char* n )
     : VotingSystem( n ){};
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
	
	static int electionsRun;
	static int simpleElections;
};

#endif
