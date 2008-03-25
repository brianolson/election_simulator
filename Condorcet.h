#ifndef CONDORCET_H
#define CONDORCET_H

#include "RankedVotePickOne.h"

class Condorcet : public RankedVotePickOne {
public:
    Condorcet()
     : RankedVotePickOne( "Condorcet" ){};
    Condorcet( const char* n )
     : RankedVotePickOne( n ){};
    virtual void runElection( int* winnerR, const VoterArray& they );
};

#endif
