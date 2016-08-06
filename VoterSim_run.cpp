#include "VoterSim.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ResultFile.h"
#include "ResultLog.h"
#include "VotingSystem.h"
#include "WorkQueue.h"

extern volatile int goGently;

const int retryLimit = 5;

void VoterSim::trialStrategySetup() {
	Voter tv(numc);
	int sti = 1;
	Strategy* st = strategies[0];
	int stlim = st->count;
	for ( int v = 0; v < numv; v++ ) {
		if ( st == NULL ) {
			theyWithError[v].setWithError( they[v], confusionError );
		} else {
			if ( doError ) {
				tv.setWithError( they[v], confusionError );
				theyWithError[v].set( tv, st );
			} else {
				theyWithError[v].set( they[v], st );
			}
			if ( --stlim == 0 ) {
				if ( sti < numStrat ) {
					st = strategies[sti];
					sti++;
					stlim = st->count;
				} else {
					st = NULL;
				}
			}
		}
	}
}

void VoterSim::trailErrorSetup() {
	for ( int v = 0; v < numv; v++ ) {
		theyWithError[v].setWithError( they[v], confusionError );
	}
}

bool VoterSim::validSetup() {
	if(!they.validate()) {
		fprintf(stderr, "some pre-error-voter failed validation with all preferences equal, pref mode: %d\n", preferenceMode);
		return false;
	}
	if(!theyWithError.validate()) {
		fprintf(stderr, "some voter-with-error failed validation with all preferences equal, pref mode: %d\n", preferenceMode);
		return false;
	}
	return true;
}

void VoterSim::preRunOutput() {
	if ( resultDump != NULL ) {
		for ( int sys = 0; sys < nsys; sys++ ) {
			fprintf( resultDump, "%s:happiness\t%s:winner\t", systems[sys]->name, systems[sys]->name );
		}
		fprintf( resultDump, "\n" );
	}
	if ( resultDumpHtml != NULL ) {
		if ( resultDumpHtmlVertical ) {
			fprintf( resultDumpHtml, "<table>\n<tr><th>Method</th><th>Happiness</th><th ALIGN=\"center\">Winner</th></tr>\n" );
		} else {
			fprintf( resultDumpHtml, "<table>\n<tr>" );
			for ( int sys = 0; sys < nsys; sys++ ) {
				fprintf( resultDumpHtml, "<th>%s:happiness</th><th>%s:winner</th>", systems[sys]->name, systems[sys]->name );
			}
			fprintf( resultDumpHtml, "</tr>\n" );
		}
	}
}

void VoterSim::preTrialOutput() {
	if ( printVoters ) {
		fprintf(text,"voters = ");
		for ( int v = 0; v < numv; v++ ) {
			they[v].print(text);
			if ( doError || strategies ) {
				theyWithError[v].print(text);
			}
			putc('\n',text);
		}
	}
	if ( voterDumpPrefix != NULL ) {
		static char* voteDumpFilename = NULL;
		if ( voteDumpFilename == NULL ) {
			voteDumpFilename = (char*)malloc( voterDumpPrefixLen + 30 );
		}
		snprintf( voteDumpFilename, voterDumpPrefixLen + 30, "%s%.10d", voterDumpPrefix, currentTrialNumber );
		voterDump( voteDumpFilename, they, numv, numc );
	}
	if ( voterBinDumpPrefix != NULL ) {
		static char* voteBinDumpFilename = NULL;
		if ( voteBinDumpFilename == NULL ) {
			voteBinDumpFilename = (char*)malloc( voterBinDumpPrefixLen + 30 );
		}
		snprintf( voteBinDumpFilename, voterBinDumpPrefixLen + 30, "%s%.10d", voterBinDumpPrefix, currentTrialNumber );
		voterBinDump( voteBinDumpFilename, they, numv, numc );
	}
	if ( resultDumpHtml != NULL ) {
		if ( ! resultDumpHtmlVertical ) {
			fprintf( resultDumpHtml, "<tr>" );
		}
	}
}

void VoterSim::postTrialOutput() {
	for ( int osys = 0; osys < nsys; osys++ ) {
		char* winnerstring = NULL;
		if ( printAllResults || (resultDump != NULL) || (resultDumpHtml != NULL) ) {
			winnerstring = new char[seats*15];
			assert(winnerstring != NULL);
			sprintf(winnerstring, "%d", winners[osys*numc + 0]);
			char* opos = winnerstring;
			for (int seat = 1; seat < seats; ++seat) {
				while (*opos != '\0') {
					opos++;
				}
				sprintf(opos, ",%d", winners[osys*numc + seat]);
			}
		}
		if ( printAllResults ) {
			fprintf(text,"%s\thappiness\t%f\twinner\t%s\n", systems[osys]->name,happiness[osys][currentTrialNumber],winnerstring);
		}
		if ( resultDump != NULL ) {
			fprintf( resultDump, "%.15g\t%s\t", happiness[osys][currentTrialNumber], winnerstring );
		}
		if ( resultDumpHtml != NULL ) {
			if ( resultDumpHtmlVertical ) {
				fprintf( resultDumpHtml, "<tr ALIGN=\"center\"><th>%s</th><td>%.15g</td><td>%s</td></tr>\n", systems[osys]->name, happiness[osys][currentTrialNumber], winnerstring );
			} else {
				fprintf( resultDumpHtml, "<td>%.15g</td><td>%s</td>", happiness[osys][currentTrialNumber], winnerstring );
			}
		}
		if (winnerstring != NULL) {
			delete [] winnerstring;
		}
	}
	if ( resultDump != NULL ) {
		fprintf( resultDump, "\n" );
	}
	if ( resultDumpHtml != NULL && ! resultDumpHtmlVertical ) {
		fprintf( resultDumpHtml, "</tr>\n" );
	}
}

void VoterSim::postRunOutput() {
	if ( resultDumpHtml != NULL ) {
		fprintf( resultDumpHtml, "</table>\n" );
		fclose( resultDumpHtml );
	}
}

void VoterSim::doResultOut(Result* resultOut) {
	if (resultOut == NULL) {
		return;
	}
	unsigned int ttot = trials + resultOut->trials;
	for ( int sys = 0; sys < nsys; sys++ ) {
		double variance;
		resultOut->systems[sys].meanHappiness = (happisum[sys] + resultOut->systems[sys].meanHappiness * resultOut->trials) / ttot;
		resultOut->systems[sys].consensusStdAvg = (happistdsum[sys] + resultOut->systems[sys].consensusStdAvg * resultOut->trials) / ttot;
		resultOut->systems[sys].giniWelfare = (ginisum[sys] + resultOut->systems[sys].giniWelfare * resultOut->trials) / ttot;
		happistdsum[sys] /= trials;
		variance = resultOut->systems[sys].reliabilityStd * resultOut->systems[sys].reliabilityStd * resultOut->trials;
		for ( int i = 0; i < trials; i++ ) {
			double d;
			d = happiness[sys][i] - resultOut->systems[sys].meanHappiness;
			variance += d * d;
		}
		variance /= ttot;
		resultOut->systems[sys].reliabilityStd = sqrt( variance );
	}
	if ( strategies ) {
		for ( int st = 0; st < numStrat; st++ ) {
			int rsys = nsys * (st + 1);
			for ( int sys = 0; sys < nsys; sys++ ) {
				double variance;
				resultOut->systems[rsys+sys].meanHappiness = (strategies[st]->happisum[sys] + resultOut->systems[rsys+sys].meanHappiness * resultOut->trials) / ttot;
				resultOut->systems[rsys+sys].giniWelfare = (strategies[st]->ginisum[sys] + resultOut->systems[rsys+sys].giniWelfare * resultOut->trials) / ttot;
				resultOut->systems[rsys+sys].consensusStdAvg = (strategies[st]->happistdsum[sys] + resultOut->systems[rsys+sys].consensusStdAvg * resultOut->trials) / ttot;
				strategies[st]->happistdsum[sys] /= trials;
				variance = resultOut->systems[rsys+sys].reliabilityStd * resultOut->systems[rsys+sys].reliabilityStd * resultOut->trials;
				for ( int i = 0; i < trials; i++ ) {
					double d;
					d = strategies[st]->happiness[sys][i] - resultOut->systems[rsys+sys].meanHappiness;
					variance += d * d;
				}
				variance /= ttot;
				resultOut->systems[rsys+sys].reliabilityStd = sqrt( variance );
			}
		}
	}
	resultOut->trials = ttot;
}

void VoterSim::oneTrial() {
	int retries = 0;

	// there are some ways that setting up a trial can fail and it's easiest to retry here
	trialretry:
	if ( goGently ) return;

	if ( strategies ) {
		trialStrategySetup();
	} else if ( doError ) {
		trailErrorSetup();
	}
	if (!validSetup()) {
		if (retries < retryLimit) {
			retries++;
			goto trialretry;
		}
		assert(false);
	}

	preTrialOutput();

	// Run elections for each system, recording all to winners.
	for (int sys = 0; sys < nsys; sys++) {
		if ( goGently ) return;
		bool ok = true;
		if ( tweValid ) {
			ok = systems[sys]->runMultiSeatElection( winners + (sys*numc), theyWithError, seats );
		} else {
			ok = systems[sys]->runMultiSeatElection( winners + (sys*numc), they, seats );
		}
		if (!ok) {
			fprintf(stderr, "sys %d failed\n", sys);
			goGently = true;
		}
		happiness[sys][currentTrialNumber] = NAN;
	}

	TrialResult logEntry;
	logEntry.set_voters(numv);
	logEntry.set_choices(numc);
	logEntry.set_error(doError ? confusionError : -1.0);
	logEntry.set_seats(seats);
	logEntry.set_voter_model(trFromVS(preferenceMode));
	logEntry.set_dimensions(dimensions);

	// Measure results for systems.
	for ( int osys = 0; osys < nsys; osys++ ) {
		//fprintf(stderr, "measure sys %d\n", osys);
		if ( isnan(happiness[osys][currentTrialNumber]) ) {
			double td, tg, th;
			th = calculateHappiness( &(winners[osys*numc/*+0*/]), &td, &tg );
			int sys = osys;
			happisum[sys] += happiness[sys][currentTrialNumber] = th;
			happistdsum[sys] += td;
			ginisum[sys] += tg;

			logEntry.set_system_index(osys);
			logEntry.set_mean_happiness(th);
			logEntry.set_voter_happiness_stddev(td);
			logEntry.set_gini_index(tg);
			bool ok = rlog->logResult(logEntry);
			if (!ok) { goGently = true; }
			for ( sys = osys+1; sys < nsys; sys++ ) {
				// Each later system that has the same result has the same resulting happiness.
				// Apply the same happiness:std:gini to them.
				bool samewinners = true;
				for (int seat = 0; seat < seats; ++seat) {
					if (winners[sys*numc + seat] != winners[osys*numc + seat]) {
						samewinners = false;
						break;
					}
				}
				if ( samewinners ) {
					happisum[sys] += happiness[sys][currentTrialNumber] = th;
					happistdsum[sys] += td;
					ginisum[sys] += tg;

					logEntry.set_system_index(sys);
					ok = rlog->logResult(logEntry);
					if (!ok) { goGently = true; }
				}
			}
		} else {
			fprintf(stderr, "non-nan result for %d\n", osys);
		}
	}

	// measure results per-strategy
	if ( strategies ) {
		double td, tg;
		for ( int sys = 0; sys < nsys; sys++ ) {
			int stpos = 0;
			for ( int st = 0; st < numStrat; st++ ) {
				strategies[st]->happisum[sys] += strategies[st]->happiness[sys][currentTrialNumber] = calculateHappiness( stpos, strategies[st]->count, &(winners[sys*numc/*+0*/]), &td, &tg );
				stpos += strategies[st]->count;
				strategies[st]->happistdsum[sys] += td;
				strategies[st]->ginisum[sys] += tg;
			}
		}
	}

	postTrialOutput();
}

void VoterSim::run( Result* resultOut ) {
	assert(rlog != NULL);
	for ( int sys = 0; sys < nsys; sys++ ) {
		happisum[sys] = 0;
		ginisum[sys] = 0;
		happistdsum[sys] = 0;
	}
	if ( doError || strategies ) {
		theyWithError.build( numv, numc );
		tweValid = true;
	}
	if ( strategies ) for ( int st = 0; st < numStrat; st++ ) {
			for ( int sys = 0; sys < nsys; sys++ ) {
				strategies[st]->happisum[sys] = 0;
				strategies[st]->ginisum[sys] = 0;
				strategies[st]->happistdsum[sys] = 0;
			}
		}

	preRunOutput();

	// Run N trial simulations
	they.build( numv, numc );
	for ( currentTrialNumber = 0; currentTrialNumber < trials; currentTrialNumber++ ) {
		if ( goGently ) return;
		if ( trials != 1 ) {
			// if trials is 1, may have loaded voters from elsewhere
			randomizeVoters();
		}
		oneTrial();
	}

	postRunOutput();
	doResultOut(resultOut);
}
