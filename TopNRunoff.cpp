#include "TopNRunoff.h"
#include "Voter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void TopNRunoff::init( const char** envp ) {
	int sp = 0, up = 0; // save pos, use pos
	const char* arg;
	if ( envp == NULL ) {
		return;
	}
	while ( (arg = envp[up]) != NULL ) {
		if ( !strcmp( arg, "N" ) ) {
			char* nn;
			up++;
			finalRoundSize = atoi( envp[up] );
			nn = (char*)malloc(20);
			if ( nn == NULL ) {
			    name = "Top N Runoff (MALLOC FAILED!)";
			} else {
			    sprintf(nn,"Top %d Runoff", finalRoundSize);
			    name = nn;
			}
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

void TopNRunoff::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    int* talley;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
    int cremain = numc;
    bool* notEliminated = new bool[numc];

    // init
    talley = new int[numc];
    for ( i = 0; i < numc; i++ ) {
        notEliminated[i] = true;
    }
    
	// fresh count
	for ( i = 0; i < numc; i++ ) {
		talley[i] = 0;
	}
	// vote first choices
	for ( i = 0; i < numv; i++ ) {
		int fav;
		float p;
		int j = 0;
		fav = j;
		p = they[i].getPref(j);
		for ( ; j < numc; j++ ) if ( they[i].getPref(j) > p ) {
			p = they[i].getPref(j);
			fav = j;
		}
		talley[fav]++;
	}
	// eliminate all but finalRoundSize
	while ( cremain > finalRoundSize ) {
		//int m;
		int loser;
		int l;
		//fprintf(stderr,"%d ", talley[0] );
		for ( i = 0; i < numc; i++ ) {
			if( notEliminated[i] )
				break;
		}
		//m = talley[i];
		l = talley[i];
		//winner = i;
		loser = i;
		for ( ; i < numc; i++ ) if ( notEliminated[i] ) {
			//fprintf(stderr,"%d ", talley[i] );
#if 0
			if ( talley[i] > m ) {
				m = talley[i];
				winner = i;
			}
#endif
			if ( talley[i] < l ) {
				l = talley[i];
				loser = i;
			}
		}
		//fprintf(stderr,"irv dq %d (winner %d)\n",loser,winner);
		notEliminated[loser] = false;
		cremain--;
	}
	// fresh count
	for ( i = 0; i < numc; i++ ) {
		talley[i] = 0;
	}
	// find first choices for non-eliminated candidates
	for ( i = 0; i < numv; i++ ) {
		int fav;
		float p;
		int j;
		for ( j = 0; j < numc; j++ ) {
			if ( notEliminated[j] )
				break;
		}
		fav = j;
		p = they[i].getPref(j);
		for ( ; j < numc; j++ ) if ( notEliminated[j] && they[i].getPref(j) > p ) {
			p = they[i].getPref(j);
			fav = j;
		}
		talley[fav]++;
	}
	// find winner
	{
		int m;
		for ( i = 0; i < numc; i++ ) {
			if( notEliminated[i] )
				break;
		}
		m = talley[i];
		winner = i;
		for ( ; i < numc; i++ ) if ( notEliminated[i] ) {
			if ( talley[i] > m ) {
				m = talley[i];
				winner = i;
			}
		}
	}
    delete [] notEliminated;
    delete [] talley;
    if ( winnerR ) *winnerR = winner;
//    return pickOneHappiness( they, numv, winner );
}

VotingSystem* newTopNRunoff( const char* n ) {
	return new TopNRunoff();
}
VSFactory* TopNRunoff_f = new VSFactory( newTopNRunoff, "TopN" );
