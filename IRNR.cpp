/*
 *  IRNR.cpp
 *  voting
 *
 *  Created by Brian Olson on Mon Dec 01 2003.
 *  Copyright (c) 2003 Brian Olson. All rights reserved.
 *
 */

#include "IRNR.h"
#include "Voter.h"
#include <stdio.h>
#include <math.h>

#ifndef TRACK_MAX
#define TRACK_MAX 0
#endif
#ifndef TRACK_DQ
#define TRACK_DQ 0
#endif

IRNR::IRNR( const char* nameIn )
: VotingSystem( nameIn ),
  offset( 0.0 )
{
}
IRNR::IRNR( const char* nameIn, double offsetIn )
: VotingSystem( nameIn ),
  offset( offsetIn )
{
}
#define rmsnorm 0
void IRNR::runElection( int* winnerR, const VoterArray& they ) const {
    bool* active;
    double* talley;
    int numActive;
    int c, v;
    double min;
    int minc;
#if TRACK_MAX
    double max;
    int maxc, oldmaxc = -1;
#endif

    active = new bool[they.numc];
    talley = new double[they.numc];
    for ( c = 0; c < they.numc; c++ ) {
		active[c] = true;
		talley[c] = 0.0;
    }
    numActive = they.numc;
	/* do the first count, normalized, all active. */
    for ( v = 0; v < they.numv; v++ ) {
		double ts;
		ts = 0.0;
		if ( rmsnorm ) {
			for ( c = 0; c < they.numc; c++ ) {
				double tts;
				tts = they[v].getPref( c ) + offset;
				ts += tts * tts;
			}
		} else {
			for ( c = 0; c < they.numc; c++ ) {
				ts += fabs( they[v].getPref( c ) + offset );
			}
		}
		if ( ts == 0.0 ) {
			// if total is zero, no talley will be added to
			// also, don't divide by zero
			continue;
		}
		if ( rmsnorm ) {
			ts = sqrt( ts );
		}
		for ( c = 0; c < they.numc; c++ ) {
			talley[c] += (they[v].getPref( c ) + offset) / ts;
		}
    }
	/* repeat until done (break out of the middle) */
    do {
		minc = -1;
		min = they.numv * 2.0;
#if TRACK_MAX
		maxc = -1;
		max = they.numv * -2.0;
#endif
		for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
			if ( talley[c] < min ) {
				min = talley[c];
				minc = c;
			}
#if TRACK_MAX
			if ( talley[c] > max ) {
				max = talley[c];
				maxc = c;
			}
#endif	
		}
#if TRACK_MAX
		if ( maxc != oldmaxc ) {
			printf( "IRNR: maxc changed from %d to %d\n", oldmaxc, maxc );
			oldmaxc = maxc;
		}
#endif
		if ( minc == -1 ) {
			fprintf( stderr, "%s:%d couldn't find a loser, min %f\n", __FILE__, __LINE__, min );
			for ( c = 0; c < they.numc; c++ ) {
				fprintf( stderr, "\ttalley[%d] = %f (%s)\n", c, talley[c], active[c] ? "ON " : "off" );
			}
			break;
		}
		active[minc] = false;
#if TRACK_DQ
		printf( "IRNR: %d disqualified\n", minc );
#endif
		numActive--;
		if ( numActive == 1 ) break;
		/* recount with renormalization */
		for ( c = 0; c < they.numc; c++ ) {
			talley[c] = 0.0;
		}
		for ( v = 0; v < they.numv; v++ ) {
			double ts;
			ts = 0.0;
			for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
				if ( rmsnorm ) {
					double tts;
					tts = they[v].getPref( c ) + offset;
					ts += tts * tts;
				} else {
					ts += fabs( they[v].getPref( c ) + offset );
				}
			}
			if ( ts == 0.0 ) {
				// if total is zero, no talley will be added to
				// also, don't divide by zero
				continue;
			}
			if ( rmsnorm ) {
				ts = sqrt( ts );
			}
			for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
				talley[c] += (they[v].getPref( c ) + offset) / ts;
			}
		}
    } while ( true );
    for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
		*winnerR = c;
		break;
    }
    delete [] active;
    delete [] talley;
}

VotingSystem* newIRNR( const char* n ) {
	return new IRNR( n );
}
VSFactory* IRNR_f = new VSFactory( newIRNR, "IRNR" );
