#include "Voter.h"
#include "STV.h"

#include <assert.h>
#include <string.h>

void STV::init( const char** envp ) {
	const char* cur = *envp;
	while (cur != NULL) {
		if (0 == strncmp(cur, "seats=", 6)) {
			seatsDefault = strtol(cur + 6, NULL, 10);
		}
		envp++;
		cur = *envp;
	}
}

static inline int getFavorite(bool* notEliminated, int voteri, const VoterArray& they, int numc) {
	int j = 0;
	float p;
	int fav;
	while (j < numc) {
		if (notEliminated[j]) break;
		j++;
	}
	if (j >= numc) { return -1; }
	p = they[voteri].getPref(j);
	fav = j;
	j++;
	while (j < numc) {
		if (notEliminated[j]) {
			float tp = they[voteri].getPref(j);
			if (tp > p) {
				p = tp;
				fav = j;
			}
		}
		j++;
	}
	return fav;
}

void STV::runElection( int* winnerR, const VoterArray& they ) const {
	runMultiSeatElection( winnerR, they, seatsDefault );
}

/*
 Count first place votes.
	elect winner over quota
	deweight satisfied voters
	recount
 If insufficient winners over quota, disqualify a loser.
 */
bool STV::runMultiSeatElection( int* winnerR, const VoterArray& they, int seats ) const {
	int i;
	int numc = they.numc;
	int numv = they.numv;
	int cremain = numc;
	double* tally = new double[numc];
	bool* electionActive = new bool[numc];
	bool* roundActive = new bool[numc];
	double* weight = new double[numv];
	double quota;
	int totalvotes;
	int numwinners;
	
	// init election
	tally = new double[numc];
	for ( i = 0; i < numc; i++ ) {
		electionActive[i] = true;
	}
	for (i = 0; i < numv; ++i) { weight[i] = 1.0; }
	
	// while more active candidates than available seats
	while ( cremain > seats ) {
		// begin round, fresh count, full weights
		for ( i = 0; i < numc; i++ ) {
			roundActive[i] = electionActive[i];
			tally[i] = 0;
		}
		totalvotes = 0;
		// find first choices for non-eliminated candidates
		for ( i = 0; i < numv; i++ ) {
			int fav;
			fav = getFavorite(roundActive, i, they, numc);
			weight[i] = 1.0; // full weight on first count of a round
			tally[fav] += 1.0;
			totalvotes++;
		}
		quota = (totalvotes / (seats+1)) + 1;
		numwinners = 0;
		// find a winner, eliminate a candidate?
		// Loop as long as we're finding winners
		while (true) {
			double m = quota * 0.99999999;
			int winner = -1;
			double l;
			int loser;
			//fprintf(stderr,"%d ", tally[0] );
			for ( i = 0; i < numc; i++ ) {
				if( roundActive[i] )
					break;
			}
			if (tally[i] > m) {
				m = tally[i];
				winner = i;
			}
			l = tally[i];
			loser = i;
			for ( ; i < numc; i++ ) if ( roundActive[i] ) {
				//fprintf(stderr,"%d ", tally[i] );
				if ( tally[i] > m ) {
					m = tally[i];
					winner = i;
				}
				if ( tally[i] < l ) {
					l = tally[i];
					loser = i;
				}
			}
			if (winner != -1) {
				// promote a winner.
				double deweight = quota / m;
				for (i = 0; i < numv; ++i) {
					int fav = getFavorite(roundActive, i, they, numc);
					if (fav == winner) {
						// deweight this voter, move remaining vote to next-favorite.
						weight[i] *= deweight;
						roundActive[winner] = false;
						int nextfav = getFavorite(roundActive, i, they, numc);
						tally[nextfav] += weight[i];
						roundActive[winner] = true;
					}
				}
				winnerR[numwinners] = winner;
				numwinners++;
				if (numwinners == seats) {
					goto done;
				}
				roundActive[winner] = false;
			} else {
				// disqualify the round loser
				electionActive[loser] = false;
				cremain--;
				//fprintf(stderr,"irv dq %d (winner %d)\n",loser,winner);
				break;
			}
		}
		assert(cremain > seats);
	}
done:
	delete [] electionActive;
	delete [] roundActive;
	delete [] tally;
	delete [] weight;
	return true;
}

VotingSystem* newSTV( const char* n ) {
	return new STV();
}
VSFactory* STV_f = new VSFactory( newSTV, "STV" );

STV::~STV() {
}
