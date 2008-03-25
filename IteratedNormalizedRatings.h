#ifndef IteratedNormalizedRatings_H
#define IteratedNormalizedRatings_H
/*
 *  IteratedNormalizedRatings.h
 *  voting
 *
 * "Iterated Normalized Ratings"
 *
 *  Created by Brian Olson on Tue Feb 13 2007.
 *  Copyright (c) 2007 Brian Olson. All rights reserved.
 *
 */

#include "VotingSystem.h"

class IteratedNormalizedRatings : public VotingSystem {
public:
    IteratedNormalizedRatings( const char* name );
    IteratedNormalizedRatings( const char* name, double offsetIn );
    virtual void runElection( int* winnerR, const VoterArray& they );
	double offset;
};

#endif
