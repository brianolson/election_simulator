#include "Voter.h"
#include "CIVSP.h"

#include "PermutationIterator.h"

// Based on Andrew Myers adaptation of Condorcet's method to proportional elections
// http://www.cs.cornell.edu/w8/~andru/civs/proportional.html

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

#if 0
// inherit Condorcet single winner implementation
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
#endif

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
#if 0 /* unused */
static inline double f_limit(
		int numv, int seats, int f, int votersPreferring) {
	return (votersPreferring * (seats + 1.0)) / numv;
}
#endif

/**
 for f = 1..seats, count how many voters have an f-preference for A vs B
 
 @param seats size of set to elect
 @param they voters
 @param setA int[seats] set of possible winners to compare to setB
 @param setB int[seats]
 @param countA int[seats] count for each f value, 1..seats mapped to 0..seats-1
 @param countB int[seats] f preference counts
 @param indeciesA int[seats] scratch space
 @param prefsA float[seats] scratch space
 @param indeciesB int[seats] scratch space
 @param prefsB float[seats] scratch space
 */
static void calculateFPreferences(
		int seats, const VoterArray& they,
		const int* setA, const int* setB,
		int* countA, int* countB,
		int* indeciesA, float* prefsA,
		int* indeciesB, float* prefsB) {
	for (int v = 0; v < they.numv; ++v ) {
		they[v].getSortedPrefs(indeciesA, prefsA, setA, seats);
		they[v].getSortedPrefs(indeciesB, prefsB, setB, seats);
		for (int f = 0; f < seats; ++f) {
			double sumA = 0.0;
			double sumB = 0.0;
			for (int fi = 0; fi <= f; ++fi) {
				sumA += prefsA[fi];
				sumB += prefsB[fi];
			}
			if (sumA > sumB) {
				// voter likes top f of A better than top f of B
				countA[f]++;
			} else if (sumB > sumA) {
				// voter likes top f of B better than top f of A
				countB[f]++;
			} else {
				// voter has no preference between top f of A and B
			}
		}
	}
}

/**
 a better enum, because C++ is dumb
 */
class ABWinner {
public:
	static const ABWinner A;
	static const ABWinner B;
	static const ABWinner TIE;
	
	bool operator==(const ABWinner& b) {
		return value == b.value;
	}
	
private:
	ABWinner(int x) : value(x) {}
	int value;
};

const ABWinner ABWinner::A(1);
const ABWinner ABWinner::B(2);
const ABWinner ABWinner::TIE(0);

static ABWinner compareCounts(int numv, int seats, const int* countA, const int* countB) {
	ABWinner winner = ABWinner::TIE;
	for (int f = 0; f < seats; ++f) {
		bool validA = valid_f_preference(numv, seats, f+1, countA[f]);
		bool validB = valid_f_preference(numv, seats, f+1, countB[f]);
		if (validA) {
			if (validB) {
				if (countA[f] > countB[f]) {
					winner = ABWinner::A;
				} else if (countB[f] > countB[f]) {
					winner = ABWinner::B;
				} else {
					// leave winner alone
				}
			} else {
				winner = ABWinner::A;
			}
		} else {
			if (validB) {
				winner = ABWinner::B;
			}
		}
	}
	return winner;
}

class ActiveSet {
public:
	int* indecies;
	ActiveSet* next;
	
	ActiveSet(int seats, ActiveSet* n) : next(n) {
		indecies = new int[seats];
	}
	~ActiveSet() {
		delete [] indecies;
	}
};

static void deleteAll(ActiveSet* x) {
	ActiveSet* next;
	while (x != NULL) {
		next = x->next;
		delete x;
		x = next;
	}
}


/**
 TODO: is there a proof that a (seats+1) election will contain all the winners from a (seats) election?

 If there is, then I can build winning sets one seat at a time out from the single-seat winner.
 
 The disproof would be a case where some faction prefers B+C where before they preferred A.
 I bet that's possible.
 They vote half B>A>...C, half C>A>...B. With only 1 seat they compromise on A, but with more seats available each of B and C can make quota and be elected.
 Fooey, no simple committee building method.
 */
										 

bool CIVSP::runMultiSeatElection( int* winnerR, const VoterArray& they, int seats ) const {
	assert(winnerR != NULL);
	if (seats == 1) {
		runElection(winnerR, they);
		return true;
	}
	int permutationsChecked = 1;
	PermutationIterator pi(seats, they.numc);
	ActiveSet* actives = new ActiveSet(seats, NULL);
	int numActives = 1;
	pi.get(actives->indecies);
	assert(pi.increment());
	ActiveSet* newset = new ActiveSet(seats, NULL);
	int* newcount = new int[seats];
	int* curcount = new int[seats];
	int* iscratchA = new int[seats];
	int* iscratchB = new int[seats];
	float* fscratchA = new float[seats];
	float* fscratchB = new float[seats];
	do {
		permutationsChecked++;
		pi.get(newset->indecies);
		// compare to all actives
		ActiveSet* cur = actives;
		int newbetter = 0;
		int newworse = 0;
		while (cur != NULL) {
			calculateFPreferences(seats, they, newset->indecies, cur->indecies, newcount, curcount, iscratchA, fscratchA, iscratchB, fscratchB);
			ABWinner ab = compareCounts(they.numv, seats, newcount, curcount);
			if (ab == ABWinner::A) {
				newbetter++;
			} else if (ab == ABWinner::B) {
				newworse++;
			}
			cur = cur->next;
		}
		if (newbetter == numActives) {
			// beat everything! drop old list.
			deleteAll(actives);
			actives = newset;
			numActives = 1;
			newset = new ActiveSet(seats, NULL);
		} else if (newworse == numActives) {
			// beaten by everything. drop new one.
			// newset gets overwritten at start of loop
		} else {
			// win some, lose some, it joins the active set
			newset->next = actives;
			actives = newset;
			newset = new ActiveSet(seats, NULL);
		}
	} while (pi.increment());
	bool ok = false;
	fprintf(stderr, "CIVSP checked %d permutations\n", permutationsChecked);
	if (numActives == 1) {
		for (int i = 0; i < seats; ++i) {
			winnerR[i] = actives->indecies[i];
		}
	} else {
		fprintf(stderr, "TODO: tiebreak between %d active sets at end of CIVSP\n", numActives);
		ok = false;
	}
	
	deleteAll(actives);
	delete newset;
	delete [] newcount;
	delete [] curcount;
	delete [] iscratchA;
	delete [] iscratchB;
	delete [] fscratchA;
	delete [] fscratchB;
	return ok;
}

VotingSystem* newCIVSP( const char* n ) {
	return new CIVSP();
}
VSFactory* CIVSP_f = new VSFactory( newCIVSP, "CIVSP" );

CIVSP::~CIVSP() {
}
