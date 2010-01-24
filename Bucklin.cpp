#include "Voter.h"
#include "Bucklin.h"

#include <assert.h>

#if 0
void Bucklin::init( const char** envp ) {
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

// http://en.wikipedia.org/wiki/Bucklin_voting
// Like IRV but accumulate Nth choices on Nth round.
void Bucklin::runElection( int* winnerR, const VoterArray& they ) {
	int* tally;
	int winner = -1;
	int numc = they.numc;
	int numv = they.numv;

	int* tfav = new int[numc];
	assert(tfav);
	float* tfavp = new float[numc];
	assert(tfavp);

	// init things
	tally = new int[numc];
	assert(tally);

	for (int maxn = 1; maxn <= numc; ++maxn) {
		// per-round reset
		for ( int c = 0; c  < numc; c++ ) {
			tally[c] = 0;
		}
		// for each voter ...
		for ( int v = 0; v < numv; v++ ) {
			const Voter& tv = they[v];
			int tfavc = 0;
			tfav[0] = 0;
			tfavp[0] = tv.getPref(0);
			tfavc = 1;
			// find the top maxn choices
			for (int p = 1; p < numc; ++p) {
				int ip = tfavc - 1;
				float pref = tv.getPref(p);
				if (tfavp[ip] < pref) {
					while (tfavp[ip] < pref) {
						if (ip+1 < maxn) {
							tfavp[ip+1] = tfavp[ip];
							tfav[ip+1] = tfav[ip];
							if (ip+1 >= tfavc) {
								tfavc = ip + 2;
							}
						}
						tfavp[ip] = pref;
						tfav[ip] = p;
						if (ip == 0) {
							break;
						}
						ip--;
					}
				} else if (tfavc < maxn) {
					tfav[tfavc] = p;
					tfavp[tfavc] = pref;
					tfavc++;
				}
			}
			// count up the top maxn choices
			for (int c = 0; c < maxn; ++c) {
				assert(tfav[c] >= 0);
				assert(tfav[c] < numc);
				tally[tfav[c]]++;
			}
		}
		// find winner
		{
			int m = tally[0];
			winner = 0;
			for ( int c = 1; c < numc; c++ ) {
				if ( tally[c] > m ) {
					m = tally[c];
					winner = c;
				}
			}
			if (m > (numv/2)) {
				goto done;
			}
		}
	}
done:
	assert(winner >= 0);
	delete [] tally;
	delete [] tfav;
	delete [] tfavp;
	if ( winnerR ) *winnerR = winner;
}

VotingSystem* newBucklin( const char* n ) {
	return new Bucklin();
}
VSFactory* Bucklin_f = new VSFactory( newBucklin, "Bucklin" );

Bucklin::~Bucklin() {
}
