#ifndef STV_H
#define STV_H

#include "VotingSystem.h"

class STV : public VotingSystem {
public:
    STV() : VotingSystem( "STV" ), seats(1) {};
    virtual void init( const char** envp );
    virtual void runElection( int* winnerR, const VoterArray& they );
    virtual ~STV();

    int seats;
    virtual void setSeats(int x);
};

#endif
