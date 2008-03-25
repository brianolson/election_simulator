#ifndef RANDOM_ELECTION_H
#define RANDOM_ELECTION_H
/*
 *  RandomElection.h
 *  voting
 *
 *  Created by Brian Olson on Mon Dec 01 2003.
 *  Copyright (c) 2003 Brian Olson. All rights reserved.
 *
 */

#include "VotingSystem.h"

class RandomElection : public VotingSystem {
public:
    RandomElection( const char* name );
    virtual void runElection( int* winnerR, const VoterArray& they );
};

#endif
