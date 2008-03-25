#ifndef IRNR_H
#define IRNR_H
/*
 *  IRNR.h
 *  voting
 *
 * "Instant Runoff Normalized Ratings"
 *
 *  Created by Brian Olson on Mon Dec 01 2003.
 *  Copyright (c) 2003 Brian Olson. All rights reserved.
 *
 */

#include "VotingSystem.h"

class IRNR : public VotingSystem {
public:
    IRNR( const char* name );
    IRNR( const char* name, double offsetIn );
    virtual void runElection( int* winnerR, const VoterArray& they );
	double offset;
};

#endif
