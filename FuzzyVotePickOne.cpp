#include "FuzzyVotePickOne.h"
#include "Voter.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
// inline function to avoid double evaluation of a or b
inline double fmax( double a, double b ) {
    return (a>b)?a:b;
}
#endif

#if 01
/* v in -1.0 to 1.0 */
inline double _quantize( double v, int steps ) {
	double s_2;
    steps--;
	s_2 = ((double)steps) / 2.0;
    v = v + 1.0;
    v = v * s_2;
#if 01
	{
		int ti;
		ti = (int)(v + 0.5);
		v = ti;
	}
#elif defined(__linux__)
    v = rint(v);
#else
    v = round(v);
#endif
    v = v / s_2;
    v = v - 1.0;
    return v;
}
#define quantize( v, s ) _quantize( v, s )
#else
double quantize( double v, int steps );
#endif
#if 0
static double noq( double v, int steps ) {
	return v;
}
#endif

FuzzyVotePickOne::FuzzyVotePickOne( const char* name, int doCounterweightI, int quantizeI )
    : VotingSystem( name ), doCounterweight( doCounterweightI ), quantization(quantizeI)
{
}
void FuzzyVotePickOne::init( const char** envp ) {
	int sp = 0, up = 0; // save pos, use pos
	const char* arg;
	if ( envp == NULL ) {
		return;
	}
	while ( (arg = envp[up]) != NULL ) {
		if ( !strcmp( arg, "normalize" ) ) {
			doCounterweight = 1;
		} else if ( !strcmp( arg, "maximize" ) ) {
			doCounterweight = 2;
		} else if ( !strcmp( arg, "toN" ) ) {
			quantization = -1;
		} else if ( !strcmp( arg, "quantizeTo" ) ) {
			up++;
			quantization = atoi( envp[up] );
//		} else if ( !strcmp( arg, "" ) ) {
		} else {
			if ( up != sp ) {
				envp[sp] = arg;
			}
			sp++;
		}
		up++;
	}
	if ( sp > 0 ) {
		envp[sp] = NULL;
		VotingSystem::init( envp );
	}
}

// maximize so that strongest like/dislike is 1.0/-1.0
#define TALLEY_MAXIMIZED( q ) for ( i = 0; i < numv; i++ ) {\
	int j;\
	counterweight = 0.0;\
	for ( j = 0; j < numc; j++ ) {\
		counterweight = fmax( counterweight, fabs( q(they[i].getPref(j),tq) ) );\
	}\
	counterweight = 1/counterweight;\
	for ( j = 0; j < numc; j++ ) {\
		talley[j] += q( they[i].getPref(j), tq ) * counterweight;\
	}\
}

// each voter gets equal sum weight of 1.0
#define TALLEY_NORMALIZED( q ) for ( i = 0; i < numv; i++ ) {\
	int j;\
	counterweight = 0.0;\
	for ( j = 0; j < numc; j++ ) {\
		counterweight += fabs( q(they[i].getPref(j),tq) );\
	}\
	counterweight = 1/counterweight;\
	for ( j = 0; j < numc; j++ ) {\
		talley[j] += q( they[i].getPref(j), tq ) * counterweight;\
	}\
}

// each voter gets vote radius = 1.0 ( in numc dimensions )
#define TALLEY_RMSNORM( q ) for ( i = 0; i < numv; i++ ) {\
	int j;\
	counterweight = 0.0;\
	for ( j = 0; j < numc; j++ ) {\
		double tipj;\
		tipj = q(they[i].getPref(j),tq);\
		counterweight += tipj * tipj;\
	}\
	counterweight = 1/sqrt(counterweight);\
	for ( j = 0; j < numc; j++ ) {\
		talley[j] += q( they[i].getPref(j), tq ) * counterweight;\
	}\
}

void FuzzyVotePickOne::runElection( int* winnerR, const VoterArray& they ) {
    int i;
    double* talley;
    int winner;
    double counterweight;
    int numc = they.numc;
    int numv = they.numv;
#if 0
	double (*q)( double, int );
	
	if( quantization != 0 ) {
		q = quantize;
	} else {
		q = noq;
	}
#else
#define q( a, b ) (a)
#endif
	
	int tq;
	if ( quantization == -1 ) {
		tq = numc;
	} else {
		tq = quantization;
	}

    // init things
    talley = new double[numc];
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }

    // sum weighted votes
    switch ( doCounterweight ) {
    case 2:
	// maximize so that strongest like/dislike is 1.0/-1.0
#if 01
		if ( tq != 0 ) {
			TALLEY_MAXIMIZED( quantize )
		} else {
			TALLEY_MAXIMIZED( q )
		}
#else
	for ( i = 0; i < numv; i++ ) {
	    int j;
            counterweight = 0.0;
            for ( j = 0; j < numc; j++ ) {
                counterweight = fmax( counterweight, fabs( q(they[i].getPref(j),tq) ) );
            }
            counterweight = 1/counterweight;
            for ( j = 0; j < numc; j++ ) {
                talley[j] += q( they[i].getPref(j), tq ) * counterweight;
            }
	}
#endif
	break;
    case 1:
#if 01
		if ( tq != 0 ) {
			TALLEY_NORMALIZED( quantize )
		} else {
			TALLEY_NORMALIZED( q )
		}
#else
	// each voter gets equal sum weight of 1.0
	for ( i = 0; i < numv; i++ ) {
	    int j;
            counterweight = 0.0;
            for ( j = 0; j < numc; j++ ) {
                counterweight += fabs( q(they[i].getPref(j),tq) );
            }
            counterweight = 1/counterweight;
            for ( j = 0; j < numc; j++ ) {
                talley[j] += q( they[i].getPref(j), tq ) * counterweight;
            }
	}
#endif
	break;
    case 0:
	// raw, same as maxHappiness
		if ( tq != 0 ) {
			for ( i = 0; i < numv; i++ ) {
				for ( int j = 0; j < numc; j++ ) {
				talley[j] += quantize( they[i].getPref(j), tq );
				}
			}
		} else {
			for ( i = 0; i < numv; i++ ) {
				for ( int j = 0; j < numc; j++ ) {
					talley[j] += they[i].getPref(j);
				}
			}
		}
	break;
    }
    // find winner
    {
        double m = talley[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m = talley[i];
                winner = i;
            }
        }
    }
    delete [] talley;
    if ( winnerR ) *winnerR = winner;
}

VotingSystem* newFuzzyVotePickOne( const char* n ) {
	return new FuzzyVotePickOne( "Rated Raw", 0, 0 );
}
VSFactory* FuzzyVotePickOne_f = new VSFactory( newFuzzyVotePickOne, "Rated" );
