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
