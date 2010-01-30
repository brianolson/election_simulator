#include "Voter.h"
#include "CIVSP.h"

#if 0
void CIVSP::init( const char** envp ) {
	const char* cur = *envp;
	while (cur != NULL) {
		if (0 == strncmp(cur, "seats=", 6)) {
			seats = strtol(cur + 6, NULL, 10);
		}
		envp++;
		cur = *envp;
	}
}
#endif

void CIVSP::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    int* tally;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
    
    // init things
    tally = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        tally[i] = 0;
    }
    
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        tally[they[i].getMax()]++;
    }
    // find winner
    {
        int m = tally[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( tally[i] > m ) {
                m = tally[i];
                winner = i;
            }
        }
    }
    delete [] tally;
    if ( winnerR ) *winnerR = winner;
}

/*
numv voters
numc choices
seats


*/
/**
 * @param numv total number of voters
 * @param seats number to be elected
 * @param f number of seats to consider preferring
 * @param votersPreferring number of voters who prefer this committee to some other
 * @return true if votersPreferring-voters are valid in their preference of f choices of this committee over other committees.
 */
static inline bool valid_f_preference(
		int numv, int seats, int f, int votersPreferring) {
	return ((votersPreferring * (seats + 1.0)) / numv) >= f;
}
/**
 * Return the maximum committee size that is valid to be commented on for
 * some number of voters preferring a committee out of a total population
 * of voters electing some number of seats. In other words,
 * the 'quota' of the proportional method.
 */
static inline double f_limit(
		int numv, int seats, int f, int votersPreferring) {
	return (votersPreferring * (seats + 1.0)) / numv;
}

bool CIVSP::runMultiSeatElection( int* winnerR, const VoterArray& they, int seats ) const {
	if (seats == 1) {
		runElection(winnerR, they);
		return true;
	}
	return false;
}

VotingSystem* newCIVSP( const char* n ) {
	return new CIVSP();
}
VSFactory* CIVSP_f = new VSFactory( newCIVSP, "CIVSP" );

CIVSP::~CIVSP() {
}
