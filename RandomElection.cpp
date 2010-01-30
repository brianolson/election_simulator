/*
 *  RandomElection.cpp
 *  voting
 *
 *  Created by Brian Olson on Mon Dec 01 2003.
 *  Copyright (c) 2003 Brian Olson. All rights reserved.
 *
 */

#include "RandomElection.h"
#include "Voter.h"
#include <stdlib.h>

#ifdef USE_OLD_RAND
/* only use this if your libc doesn't have the newer better random() */
static inline long random() {
    return rand();
}
#endif

RandomElection::RandomElection( const char* nameIn )
: VotingSystem( nameIn )
{
}
void RandomElection::runElection( int* winnerR, const VoterArray& they ) const {
    *winnerR = random() % they.numc;
}


VotingSystem* newRandomElection( const char* n ) {
	return new RandomElection( n );
}
VSFactory* RandomElection_f = new VSFactory( newRandomElection, "Random" );
