#include "InstantRunoffVotePickOne.h"
#include "Voter.h"

void InstantRunoffVotePickOne::runElection( int* winnerR, const VoterArray& they ) {
    int i;
    int* talley;
    int winner = -1;
    int numc = they.numc;
    int numv = they.numv;
    int cremain = numc;
    bool* notEliminated = new bool[numc];

    // init
    talley = new int[numc];
    for ( i = 0; i < numc; i++ ) {
        notEliminated[i] = true;
    }
    
    // while not all but one candidates are eliminated
    while ( cremain > 1 ) {
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
        // eliminate a candidate, find a winner?
        {
            int m;
            int loser;
            int l;
			//fprintf(stderr,"%d ", talley[0] );
			for ( i = 0; i < numc; i++ ) {
				if( notEliminated[i] )
					break;
			}
			m = talley[i];
			l = talley[i];
			winner = i;
			loser = i;
            for ( ; i < numc; i++ ) if ( notEliminated[i] ) {
				//fprintf(stderr,"%d ", talley[i] );
                if ( talley[i] > m ) {
                    m = talley[i];
                    winner = i;
                }
                if ( talley[i] < l ) {
                    l = talley[i];
                    loser = i;
                }
            }
			//fprintf(stderr,"irv dq %d (winner %d)\n",loser,winner);
            notEliminated[loser] = false;
            cremain--;
        }
    }
    delete [] notEliminated;
    delete [] talley;
    if ( winnerR ) *winnerR = winner;
}

VotingSystem* newInstantRunoffVotePickOne( const char* n ) {
	return new InstantRunoffVotePickOne();
}
VSFactory* InstantRunoffVotePickOne_f = new VSFactory( newInstantRunoffVotePickOne, "IRV" );
