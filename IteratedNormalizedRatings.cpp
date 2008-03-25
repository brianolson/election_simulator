/*
 *  IteratedNormalizedRatings.cpp
 *  voting
 *
 *  Created by Brian Olson on Tue Feb 13 2007.
 *  Copyright (c) 2007 Brian Olson. All rights reserved.
 *
 */

#include "IteratedNormalizedRatings.h"
#include "Voter.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef TRACK_MAX
#define TRACK_MAX 0
#endif
#ifndef TRACK_DQ
#define TRACK_DQ 0
#endif

IteratedNormalizedRatings::IteratedNormalizedRatings( const char* nameIn )
: VotingSystem( nameIn ),
  offset( 0.0 )
{
}
IteratedNormalizedRatings::IteratedNormalizedRatings( const char* nameIn, double offsetIn )
: VotingSystem( nameIn ),
  offset( offsetIn )
{
}

/* 
   Decrement methods
 */
static int decrementLowest( double* candDeWeight, double* tally, bool* active, int numc, double epsilon ) {
    int mini;
    double min;
    int i = 0;
    int numActive = 0;
    while ( ! active[i] ) {
	i++;
	numActive++;
	assert(i < numc);
    }
    min = tally[i];
    mini = i;
    i++;
    while ( i < numc ) {
	if ( active[i] ) {
	    numActive++;
	    if ( tally[i] < min ) {
		min = tally[i];
		mini = i;
	    }
	}
	i++;
    }
    candDeWeight[mini] -= epsilon;
    if ( candDeWeight[mini] < 0.0 ) {
	active[mini] = false;
	numActive--;
#if TRACK_DQ
	printf( "IteratedNormalizedRatings: %d disqualified\n", mini );
#endif
    }
    return numActive;
}
static int decrementSliding( double* candDeWeight, double* tally, bool* active, int numc, double epsilon ) {
    int i = 0;
    int numActive = 0;
    double* mins = new double[numc];
    int* minis = new int[numc];
    for ( i = 0; i < numc; i++ ) {
	if ( active[i] ) {
	    int ii = numActive-1;
	    // insert sort, move greater to the back
	    while ( (ii >= 0) && (tally[i] < mins[ii]) ) {
		mins[ii+1] = mins[ii];
		minis[ii+1] = minis[ii];
		ii--;
	    }
	    ii++;
	    mins[ii] = tally[i];
	    minis[ii] = i;
	    numActive++;
	}
    }
    for ( i = 0; i < numActive-1; i++ ) {
	candDeWeight[minis[i]] -= epsilon * ((numActive-i*1.0)/numActive);
	if ( candDeWeight[minis[i]] < 0.0 ) {
	    active[minis[i]] = false;
	    numActive--;
#if TRACK_DQ
	    printf( "IteratedNormalizedRatings: %d disqualified\n", minis[i] );
#endif
	}
    }
    delete [] minis;
    delete [] mins;
    return numActive;
}
static int decrementProportional( double* candDeWeight, double* tally, bool* active, int numc, double epsilon ) {
    int i = 0;
    int numActive = 0;
    double* mins = new double[numc];
    int* minis = new int[numc];
    double maxTally = -HUGE_VAL;
    for ( i = 0; i < numc; i++ ) {
	if ( active[i] ) {
	    int ii = numActive-1;
	    if ( tally[i] > maxTally ) {
		maxTally = tally[i];
	    }
	    // insert sort, move greater to the back
	    while ( (ii >= 0) && (tally[i] < mins[ii]) ) {
		mins[ii+1] = mins[ii];
		minis[ii+1] = minis[ii];
		ii--;
	    }
	    ii++;
	    mins[ii] = tally[i];
	    minis[ii] = i;
	    numActive++;
	}
    }
    for ( i = 0; i < numActive-1; i++ ) {
	candDeWeight[minis[i]] -= epsilon * ((maxTally-tally[minis[i]])/maxTally);
	if ( candDeWeight[minis[i]] < 0.0 ) {
	    active[minis[i]] = false;
	    numActive--;
#if TRACK_DQ
	    printf( "IteratedNormalizedRatings: %d disqualified\n", minis[i] );
#endif
	}
    }
    delete [] minis;
    delete [] mins;
    return numActive;
}

struct INR_decrOptionsType {
    int (*fun)(double*,double*,bool*,int,double);
    const char* name;
} INR_decrOptions[] = {
    { decrementProportional, "decrementProportional" },
    { decrementSliding, "decrementSliding" },
    { decrementLowest, "decrementLowest" },
    { NULL, NULL }
};

#define rmsnorm 0
void IteratedNormalizedRatings::runElection( int* winnerR, const VoterArray& they ) {
    bool* active;
    double* tally;
    double* candDeWeight;
    int numActive;
    int c, v;
    //double min;
    //int minc;
#if TRACK_MAX
    double max;
    int maxc, oldmaxc = -1;
#endif
    //double maxtally;

    active = new bool[they.numc];
    tally = new double[they.numc];
    candDeWeight = new double[they.numc];
    for ( c = 0; c < they.numc; c++ ) {
	active[c] = true;
	tally[c] = 0.0;
	candDeWeight[c] = 1.0;
    }
    //numActive = they.numc;
    for ( v = 0; v < they.numv; v++ ) {
	double ts;
	ts = 0.0;
	if ( rmsnorm ) {
		for ( c = 0; c < they.numc; c++ ) {
			double tts;
			tts = they[v].getPref( c ) + offset;
			tts *= candDeWeight[c];
			ts += tts * tts;
		}
	} else {
		for ( c = 0; c < they.numc; c++ ) {
			ts += fabs( they[v].getPref( c ) + offset ) * candDeWeight[c];
		}
	}
	if ( ts == 0.0 ) {
		// if total is zero, no tally will be added to
		// also, don't divide by zero
		continue;
	}
	if ( rmsnorm ) {
		ts = sqrt( ts );
	}
	for ( c = 0; c < they.numc; c++ ) {
	    tally[c] += (they[v].getPref( c ) + offset) * candDeWeight[c] / ts;
	}
    }
    do {
#if 1
	numActive = decrementSliding( candDeWeight, tally, active, they.numc, 0.10 );
#else
	minc = -1;
	min = they.numv * 2.0;
#if TRACK_MAX
	maxc = -1;
	max = they.numv * -2.0;
#endif
	for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
	    if ( tally[c] < min ) {
		min = tally[c];
		minc = c;
	    }
#if TRACK_MAX
	    if ( tally[c] > max ) {
		max = tally[c];
		maxc = c;
	    }
#endif	
	}
#if TRACK_MAX
	if ( maxc != oldmaxc ) {
	    printf( "IteratedNormalizedRatings: maxc changed from %d to %d\n", oldmaxc, maxc );
	    oldmaxc = maxc;
	}
#endif
	if ( minc == -1 ) {
	    fprintf( stderr, "%s:%d couldn't find a loser, min %f\n", __FILE__, __LINE__, min );
	    for ( c = 0; c < they.numc; c++ ) {
		fprintf( stderr, "\ttally[%d] = %f (%s)\n", c, tally[c], active[c] ? "ON " : "off" );
	    }
	    break;
	}
	active[minc] = false;
#if TRACK_DQ
	printf( "IteratedNormalizedRatings: %d disqualified\n", minc );
#endif
	numActive--;
#endif
	if ( numActive == 1 ) break;
	for ( c = 0; c < they.numc; c++ ) {
	    tally[c] = 0.0;
	}
	for ( v = 0; v < they.numv; v++ ) {
	    double ts;
	    ts = 0.0;
	    for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
		if ( rmsnorm ) {
			double tts;
			tts = they[v].getPref( c ) + offset;
			tts *= candDeWeight[c];
			ts += tts * tts;
		} else {
		    ts += fabs( they[v].getPref( c ) + offset ) * candDeWeight[c];
		}
	    }
		if ( ts == 0.0 ) {
			// if total is zero, no tally will be added to
			// also, don't divide by zero
			continue;
		}
	    for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
		tally[c] += (they[v].getPref( c ) + offset) * candDeWeight[c] / ts;
	    }
	}
    } while ( true );
    for ( c = 0; c < they.numc; c++ ) if ( active[c] ) {
	*winnerR = c;
	break;
    }
    delete [] active;
    delete [] tally;
    delete [] candDeWeight;
}

VotingSystem* newIteratedNormalizedRatings( const char* n ) {
	return new IteratedNormalizedRatings( n );
}
VSFactory* IteratedNormalizedRatings_f = new VSFactory( newIteratedNormalizedRatings, "IteratedNormalizedRatings" );
