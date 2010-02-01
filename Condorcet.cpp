#include "Voter.h"
#include "Condorcet.h"
#include <stdio.h>
#include <time.h>


inline unsigned int umin( unsigned int a, unsigned int b ) {
    if ( a < b ) {
	return a;
    } else {
	return b;
    }
}
/**
 * bpm Beat Path Matrix, filled in as we go.
 */
void runBeatpath( const unsigned int* talley, unsigned int* bpm, int numc, int depth ) {
    int j, k;
    // foreach pair of candidates
    bool notDone = true;
    for ( j = 0; j < numc; j++ ) {
	bpm[j*numc + j] = 0;
	for ( k = j + 1; k < numc; k++ ) {
	    int vj, vk;
	    vk = talley[k*numc + j];	// k beat j vk times
	    vj = talley[j*numc + k];	// j beat k vj times
	    if ( vk > vj ) {
		bpm[k*numc + j] = vk;
		bpm[j*numc + k] = 0;
	    } else if ( vj > vk ) {
		bpm[k*numc + j] = 0;
		bpm[j*numc + k] = vj;
	    } else /* vj == vk */ {
#if 01
		bpm[k*numc + j] = 0;
		bpm[j*numc + k] = 0;
#endif
	    }
	}
    }

    /* Condorcet sucks computationally because this set of nested loops
     * can be as bad as N^4 in the number of choices */
    while ( notDone ) {
	notDone = false;
#if 0
	for ( j = 0; j < numc; j++ ) {
	    for ( k = 0; k < numc; k++ ) {
		printf("%d\t", bpm[j*numc + k] );
	    }
	    printf("\n");
	}
	printf("\n");
#endif
	for ( j = 0; j < numc; j++ ) {
	    for ( k = 0; k < numc; k++ ) if ( k != j ) {
		int vk;
		vk = bpm[k*numc + j];	// candidate k > j
		if ( vk != 0 ) {
		    // sucessful beat, see if it can be used to get a better beat over another
		    for ( int l = 0; l < numc; l++ ) if ( l != j && l != k ) { // don't care about self (k) or same (j)
			unsigned int vl;
			vl = umin( bpm[j*numc + l], vk );	// j > l
			if ( /*vl != 0 &&*/ vl > bpm[k*numc + l] ) {
			    // better beat path found
//			    printf("set bpm[%d * %d + %d] = %d\n", k, numc, l, vl);
			    bpm[k*numc + l] = vl;
			    notDone = true;
			}
		    }
		}
	    }
	}
    }
    for ( j = 0; j < numc; j++ ) {
	for ( k = j + 1; k < numc; k++ ) {
	    int vj, vk;
	    vk = bpm[k*numc + j];
	    vj = bpm[j*numc + k];
	    if ( vk > vj ) {
		//bpm[k*numc + j] = vk;
		bpm[j*numc + k] = 0;
	    } else if ( vj > vk ) {
		bpm[k*numc + j] = 0;
		//bpm[j*numc + k] = vj;
	    }
	}
    }
#if 0
    for ( j = 0; j < numc; j++ ) {
	for ( k = 0; k < numc; k++ ) {
	    printf("%d\t", bpm[j*numc + k] );
	}
	printf("\n");
    }
#endif
}

int Condorcet::electionsRun = 0;
int Condorcet::simpleElections = 0;

void Condorcet::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    unsigned int* talley;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
    
	electionsRun++;
	
    // init things
    // talley is a matrix of who beats who
    talley = new unsigned int[numc*numc];
    for ( i = 0; i  < numc*numc; i++ ) {
        talley[i] = 0;
    }
    
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        int j, k;
        const Voter* v;
        v = &(they[i]);
	// foreach pair of candidates, vote 1-on-1
        for ( j = 0; j < numc; j++ ) {
	    for ( k = j + 1; k < numc; k++ ) {
		double pk, pj;
		pk = v->getPref(k);
		pj = v->getPref(j);
		if ( pk > pj ) {
		    talley[k*numc + j]++;	// k beats j
		} else if ( pj > pk ) {
		    talley[j*numc + k]++;	// j beats k
		}
            }
        }
    }
    // find winner
    {
	unsigned int* defeatCount;
	/* ndefeats is "numc choose 2" or ((numc !)/((2 !)*((numc - 2)!))) */
#if 0
	int ndefeats = (numc*(numc-1))/2;
	int dpos = 0;
#define COUNT_DEFEATS 1
#define setNDefeats( n ) ndefeats = (n)
#else
#define setNDefeats( n )
#endif
	int j,k;
	
	defeatCount = new unsigned int[numc];
        for ( j = 0; j < numc; j++ ) {
	    defeatCount[j] = 0;
	}
        for ( j = 0; j < numc; j++ ) {
	    for ( k = j + 1; k < numc; k++ ) {
		unsigned int vk, vj;
		vk = talley[k*numc + j];	// k beat j vk times
		vj = talley[j*numc + k];	// j beat k vj times
		if ( vj > vk ) {
		    defeatCount[k]++;
		} else if ( vj < vk ) {
		    defeatCount[j]++;
		}
	    }
	}
	setNDefeats( dpos );
	winner = -1;
        for ( j = 0; j < numc; j++ ) {
	    if ( defeatCount[j] == 0 ) {
		winner = j;
			simpleElections++;
		break;
	    }
	}
	if ( winner == -1 ) {
	    unsigned int* bpm;
	    int winsize = 0;
	    bpm = new unsigned int[numc*numc];
	    runBeatpath( talley, bpm, numc, 0 );
	    for ( j = 0; j < numc; j++ ) {
		int winsizet;
		winsizet = 0;
		defeatCount[j] = 0;
		for ( k = 0; k < numc; k++ ) if ( k != j ) {
		    int bpmt = bpm[j*numc + k];
		    if ( bpmt == 0 ) {
			defeatCount[j]++;
		    } else if ( bpmt > winsizet ) {
			winsizet = bpmt;
		    }
		}
		if ( defeatCount[j] == 0 ) {
		    if ( winner == -1 ) {
			winner = j;
			winsize = winsizet;
		    } else if ( winsizet > winsize ) {
			winner = j;
			winsize = winsizet;
		    } else if ( winsizet == winsize ) {
#if 0
			// FIXME write an option to return ties
#if COUNT_DEFETAS
			for ( j = 0; j < ndefeats; j++ ) {
			    printf("defeat[%d] %d > %d (%d/%d)\n", j, defeats[j].winner, defeats[j].loser, defeats[j].winnerVotes, defeats[j].loserVotes );
			}
#endif
			for ( j = 0; j < numc; j++ ) {
			    for ( k = 0; k < numc; k++ ) {
				printf("%d\t", bpm[j*numc + k] );
			    }
			    printf("\n");
			}
			printf("muliptle Condorcet-beatpath winners, going with first\n");
			break;
#endif
		    }
		}
	    }
	    if ( winner == -1 ) {
		static char cErrFileName[32];
		snprintf( cErrFileName, sizeof(cErrFileName), "condorcetError%.10ld", time(NULL) );
		FILE* fo = fopen( cErrFileName, "w" );
		if ( fo == NULL ) {
		    perror( cErrFileName );
		} else {
		fprintf( fo, "no Condorcet-beatpath winner, bpm array:\n");
		for ( j = 0; j < numc; j++ ) {
		    fprintf( fo, "dc(%d)\t", defeatCount[j] );
		    for ( k = 0; k < numc; k++ ) {
			fprintf( fo, "%d\t", bpm[j*numc + k] );
		    }
		    fprintf( fo, "\n");
		}
		fprintf( fo, "\ntalley array:\n");
		for ( j = 0; j < numc; j++ ) {
		    for ( k = 0; k < numc; k++ ) {
			fprintf( fo, "%d\t", talley[j*numc + k] );
		    }
		    fprintf( fo, "\n");
		}
		fprintf( fo, "\n");
		voterDump( fo, they, numv, numc );
		fclose(fo);
		}
	    }
	    delete [] bpm;
	}
	if ( winner == -1 ) {
#if 0
	    // FIXME write an option to return ties
#elif 0
	    // another alternative, random pick from tie (not for real-world)
#else
	    fprintf(stderr, "no Condorcet-beatpath winner, going with 0\n");
	    winner = 0;
#endif
	}
	delete [] defeatCount;
    }
    delete [] talley;
    if ( winner != -1 && winnerR ) *winnerR = winner;
}

VotingSystem* newCondorcet( const char* n ) {
	return new Condorcet( n );
}
VSFactory* Condorcet_f = new VSFactory( newCondorcet, "Condorcet" );
