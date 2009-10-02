#include "Voter.h"
#include "IRNRP.h"
#include "MaybeDebugLog.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void IRNRP::init( const char** envp ) {
	const char* cur = *envp;
	while (cur != NULL) {
		if (0 == strncmp(cur, "seats=", 6)) {
			seatsDefault = strtol(cur + 6, NULL, 10);
		} else if (0 == strncmp(cur, "debug=", 6)) {
			debug = MaybeDebugLog::ForPath(cur + 6);
		}
		envp++;
		cur = *envp;
	}
}

/*
 If a legallistic definiton specified 'not more than one part in one thousand'
 or similar the implementation might need to be redone in integer millivotes
 or some specific integer fraction thereof.
 */
static const double overtallyEpsilon = 1.001;


void IRNRP::runElection( int* winnerR, const VoterArray& they ) {
	runMultiSeatElection(winnerR, they, seatsDefault);
}

/*
 Sum normalized ratings
 Calculate quota
 Deweight over-quota choices
 If over-quota sum less than epsilon and insufficient winners, disqualify lowest.
 */
bool IRNRP::runMultiSeatElection( int* winnerR, const VoterArray& they, int seats ) {
	int i;
	int numc = they.numc;
	int numv = they.numv;
	double* tally = new double[numc];
	double* cweight = new double[numc];
	bool* active = new bool[numc];
	bool* winning = new bool[numc];
	int roundcounter = 1;
	int numactive = numc;
	
	for (int c = 0; c < numc; ++c) {
		active[c] = true;
		winning[c] = false;
		cweight[c] = 1.0;
	}
	if (debug) {
		debug->clear();
		debug->setEnable(false);
		debug->vlog("<table>");
	}

	while (true) {
		// round init
		for (int c = 0; c < numc; ++c) {
			tally[c] = 0.0;
		}
		
		// count normalized votes for each candidate
		for ( i = 0; i < numv; i++ ) {
			double vsum = 0.0;
			double minp = 0.0;
			bool shiftvotes = false;
			for (int c = 0; c < numc; ++c) {
				if (active[c]) {
					double p = they[i].getPref(c);
					if (p < minp) {
						minp = p;
						shiftvotes = true;
					}
				}
			}
			if (shiftvotes) {
				// shift all values positive. negatives break quota.
				vsum = 0.0;
				for (int c = 0; c < numc; ++c) {
					if (active[c]) {
						double p = they[i].getPref(c) - minp;
						double t = p * cweight[c];
						vsum += t * t;
					}
				}
				if (vsum <= 0.0) {
					assert(false);
					continue;
				}
				vsum = 1.0 / sqrt(vsum);
				for (int c = 0; c < numc; ++c) {
					if (active[c]) {
						double p = they[i].getPref(c) - minp;
						assert(p >= 0.0);
						tally[c] += p * cweight[c] * vsum;
					}
				}
			} else {
				for (int c = 0; c < numc; ++c) {
					if (active[c]) {
						double p = they[i].getPref(c);
						double t = p * cweight[c];
						vsum += t * t;
					}
				}
				if (vsum <= 0.0) {
					assert(false);
					continue;
				}
				vsum = 1.0 / sqrt(vsum);
				for (int c = 0; c < numc; ++c) {
					if (active[c]) {
						double p = they[i].getPref(c);
						assert(p >= 0.0);
						tally[c] += p * cweight[c] * vsum;
					}
				}
			}
		}
		double totalvote = 0.0;
		for (int c = 0; c < numc; ++c) {
			if (active[c]) {
				assert(tally[c] > 0.0);
				totalvote += tally[c];
			}
		}
		double quota = totalvote / (seats + 1.0);
		double quotaPlusEpsilon = quota * overtallyEpsilon;
		//fprintf(stderr, "%d total vote=%f, quota for %d seats is %f\n", roundcounter, totalvote, seats, quota);
		int numwinners = 0;
		bool cweightAdjusted = false;
		for (int c = 0; c < numc; ++c) {
			if (tally[c] > quota) {
				winning[c] = true;
				numwinners++;
				if (tally[c] > quotaPlusEpsilon) {
					//fprintf(stderr, "%d tally[%d]=%f > %f. old cweight=%f\n", roundcounter, c, tally[c], quotaPlusEpsilon, cweight[c]);
					cweight[c] *= (quota / tally[c]);
					cweightAdjusted = true;
					//fprintf(stderr, "%d tally[%d]=%f > %f. new cweight=%f\n", roundcounter, c, tally[c], quotaPlusEpsilon, cweight[c]);
				}
			} else if (winning[c]) {
				//numwinners++;
				if (debug) {
					static int fail_limit = 4;
					if (--fail_limit < 0) {
						delete debug;
						exit(1);
					}
					debug->setEnable(true);
				fprintf(
					stderr, "%d warning, winning choice(%d) has fallen below quota (%f < %f)\n",
					roundcounter, c, tally[c], quota);
				}
				winning[c] = false;
			}
		}
		if (debug) {
			debug->vlog("<tr><td class=\"r\">%d</td><td class=\"q\">%f</td>", roundcounter, quota);
			for (int c = 0; c < numc; ++c) {
				debug->vlog("<td class=\"%s\">* %f = %f</td>",
					winning[c] ? (tally[c] < quota ? "wine" : "win") : (active[c] ? "ne" : "dq"),
					cweight[c], tally[c]);
			}
			debug->vlog("</tr>\n");
		}
#if 0
		for (int c = 0; c < numc; ++c) {
			fprintf(stderr, "%d tally[%d] = %f, cweight=>%f\n", roundcounter, c, tally[c], cweight[c]);
		}
#endif
		if (numwinners == seats) {
			break;
		}
		if (cweightAdjusted) {
			// re-run with new weights
		} else {
			// disqualify a loser.
			double mint = 9999999.0;
			int mini = -1;
			for (int c = 0; c < numc; ++c) {
				if (active[c]) {
					if ((mini == -1) || (tally[c] < mint)) {
						mint = tally[c];
						mini = c;
					}
				}
			}
			assert(mini != -1);
			active[mini] = false;
			numactive--;
			if (numactive == seats) {
				// that which remains, wins.
				for (int c = 0; c < numc; ++c) {
					winning[c] = active[c];
				}
				break;
			}
			// reset cweights
			for (int c = 0; c < numc; ++c) {
				cweight[c] = 1.0;
			}
		}
		roundcounter++;
	}
	if (debug) {
		debug->vlog("</table>\n");
		if (!debug->isEnabled()) {
			debug->clear();
		}
	}
	int winneri = 0;
	for (int c = 0; c < numc; ++c) {
		if (winning[c]) {
			winnerR[winneri] = c;
			winneri++;
		}
	}
	delete [] winning;
	delete [] active;
	delete [] cweight;
	delete [] tally;
	return true;
}

VotingSystem* newIRNRP( const char* n ) {
	return new IRNRP();
}
VSFactory* IRNRP_f = new VSFactory( newIRNRP, "IRNRP" );

IRNRP::~IRNRP() {
	if (debug != NULL) {
		delete debug;
	}
}
