#include "NNStrategicVoter.h"
#include "VoterSim.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "ResultFile.h"
#include "DBResultFile.h"
#include "VotingSystem.h"
#include "WorkQueue.h"

#include "NNSVSim.h"

extern int goGently;

#if 0
void NNSVSim::run( Result* r ) {
	/* a naked body of a function, define things, #include this */
	int i;
	int sys;
	if ( r == NULL ) {
		return;
	}
	for ( sys = 0; sys < nsys; sys++ ) {
		happisum[sys] = 0;
		happistdsum[sys] = 0;
	}
	if ( doError || strategies ) {
		theyWithError.build( numv, numc );
		tweValid = true;
	}
#if (!defined(strategies)) || strategies
	if ( strategies ) for ( int st = 0; st < numStrat; st++ ) {
		for ( sys = 0; sys < nsys; sys++ ) {
			strategies[st]->happisum[sys] = 0;
			strategies[st]->happistdsum[sys] = 0;
		}
	}
#endif
	if ( resultDump != NULL ) {
		for ( sys = 0; sys < nsys; sys++ ) {
			fprintf( resultDump, "%s:happiness\t%s:winner\t", systems[sys]->name, systems[sys]->name );
		}
		fprintf( resultDump, "\n" );
	}
	if ( resultDumpHtml != NULL ) {
		if ( resultDumpHtmlVertical ) {
			fprintf( resultDumpHtml, "<table>\n<tr><th>Method</th><th>Happiness</th><th ALIGN=\"center\">Winner</th></tr>\n" );
		} else {
			fprintf( resultDumpHtml, "<table>\n<tr>" );
			for ( sys = 0; sys < nsys; sys++ ) {
				fprintf( resultDumpHtml, "<th>%s:happiness</th><th>%s:winner</th>", systems[sys]->name, systems[sys]->name );
			}
			fprintf( resultDumpHtml, "</tr>\n" );
		}
	}
	they.build( numv, numc );
	for ( i = 0; i < trials; i++ ) {
		if ( goGently ) return;
		if ( trials != 1 ) {
			for ( int v = 0; v < numv; v++ ) {
				they[v].randomize();
			}
		}
#if (!defined(strategies)) || strategies
		if ( strategies ) {
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
		} else
#endif
			if ( doError ) {
				for ( int v = 0; v < numv; v++ ) {
					theyWithError[v].setWithError( they[v], confusionError );
				}
			}
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
#if (!defined(voterDumpPrefix)) || voterDumpPrefix
		if ( voterDumpPrefix != NULL ) {
			static char* voteDumpFilename = NULL;
			if ( voteDumpFilename == NULL ) {
				voteDumpFilename = (char*)malloc( voterDumpPrefixLen + 30 );
			}
			snprintf( voteDumpFilename, voterDumpPrefixLen + 30, "%s%.10d", voterDumpPrefix, i );
			voterDump( voteDumpFilename, they, numv, numc );
		}
#endif
#if (!defined(voterBinDumpPrefix)) || voterBinDumpPrefix
		if ( voterBinDumpPrefix != NULL ) {
			static char* voteBinDumpFilename = NULL;
			if ( voteBinDumpFilename == NULL ) {
				voteBinDumpFilename = (char*)malloc( voterBinDumpPrefixLen + 30 );
			}
			snprintf( voteBinDumpFilename, voterBinDumpPrefixLen + 30, "%s%.10d", voterBinDumpPrefix, i );
			voterBinDump( voteBinDumpFilename, they, numv, numc );
		}
#endif
		if ( resultDumpHtml != NULL ) {
			if ( ! resultDumpHtmlVertical ) {
				fprintf( resultDumpHtml, "<tr>" );
			}
		}
		for ( sys = 0; sys < nsys; sys++ ) {
			double td;
			if ( goGently ) return;
			if ( tweValid ) {
				systems[sys]->runElection( winners, theyWithError );
			} else {
				systems[sys]->runElection( winners, they );
			}
			happisum[sys] += happiness[sys][i] = VotingSystem::pickOneHappiness( they, numv, *winners, &td );
			happistdsum[sys] += td;
#if (!defined(strategies)) || strategies
			if ( strategies ) {
				int stpos = 0;
				for ( int st = 0; st < numStrat; st++ ) {
					strategies[st]->happisum[sys] += strategies[st]->happiness[sys][i] = VotingSystem::pickOneHappiness( they, strategies[st]->count, *winners, &td, stpos );
					stpos += strategies[st]->count;
					strategies[st]->happistdsum[sys] += td;
				}
			}
#endif
			if ( printAllResults ) {
				fprintf(text,"%s\thappiness\t%f\twinner\t%d\n", systems[sys]->name,happiness[sys][i],*winners);
				//                fprintf(text,"");
			}
			if ( resultDump != NULL ) {
				fprintf( resultDump, "%.15g\t%d\t", happiness[sys][i], *winners );
			}
			if ( resultDumpHtml != NULL ) {
				if ( resultDumpHtmlVertical ) {
					fprintf( resultDumpHtml, "<tr ALIGN=\"center\"><th>%s</th><td>%.15g</td><td>%d</td></tr>\n", systems[sys]->name, happiness[sys][i], *winners );
				} else {
					fprintf( resultDumpHtml, "<td>%.15g</td><td>%d</td>", happiness[sys][i], *winners );
				}
			}
		}
		if ( resultDump != NULL ) {
			fprintf( resultDump, "\n" );
		}
		if ( resultDumpHtml != NULL && ! resultDumpHtmlVertical ) {
			fprintf( resultDumpHtml, "</tr>\n" );
		}
		if ( i+1 < trials ) {
			for ( int v = 0; v < numv; v++ ) {
				they[v].randomize();
			}
		}
	}
	if ( resultDumpHtml != NULL ) {
		fprintf( resultDumpHtml, "</table>\n" );
		fclose( resultDumpHtml );
	}
	unsigned int ttot = trials + r->trials;
	for ( sys = 0; sys < nsys; sys++ ) {
		double variance;
		r->systems[sys].meanHappiness = (happisum[sys] + r->systems[sys].meanHappiness * r->trials) / ttot;
		r->systems[sys].consensusStdAvg = (happistdsum[sys] + r->systems[sys].consensusStdAvg * r->trials) / ttot;
		happistdsum[sys] /= trials;
		variance = r->systems[sys].reliabilityStd * r->systems[sys].reliabilityStd * r->trials;
		for ( i = 0; i < trials; i++ ) {
			double d;
			d = happiness[sys][i] - r->systems[sys].meanHappiness;
			variance += d * d;
		}
		variance /= ttot;
		r->systems[sys].reliabilityStd = sqrt( variance );
	}
#if (!defined(strategies)) || strategies
	if ( strategies ) for ( int st = 0; st < numStrat; st++ ) {
		int rsys = nsys * (st + 1);
		for ( sys = 0; sys < nsys; sys++ ) {
			double variance;
			r->systems[rsys+sys].meanHappiness = (strategies[st]->happisum[sys] + r->systems[rsys+sys].meanHappiness * r->trials) / ttot;
			r->systems[rsys+sys].consensusStdAvg = (strategies[st]->happistdsum[sys] + r->systems[rsys+sys].consensusStdAvg * r->trials) / ttot;
			strategies[st]->happistdsum[sys] /= trials;
			variance = r->systems[rsys+sys].reliabilityStd * r->systems[rsys+sys].reliabilityStd * r->trials;
			for ( i = 0; i < trials; i++ ) {
				double d;
				d = strategies[st]->happiness[sys][i] - r->systems[rsys+sys].meanHappiness;
				variance += d * d;
			}
			variance /= ttot;
			r->systems[rsys+sys].reliabilityStd = sqrt( variance );
		}
	}
#endif
		r->trials = ttot;
}
#endif

void NNSVSim::initPop( int n, int c ) {
	numv = n;
	numc = c;
	they.build( numv, numc );
	for ( int v = 0; v < numv; v++ ) {
		they[v].setHonest();//randomizeNN();
	}
}
// run one generation
void NNSVSim::testg( int trialsPerGeneration ) {
	double poll[numc];
	int winners[numc];
	for ( int c = 0; c < numc; c++ ) {
		poll[c] = 0;
	}
	for ( int v = 0; v < numv; v++ ) {
		they[v].strategySuccess = 0.0;
	}
	for ( int t = 0; t < trialsPerGeneration; t++ ) {
		if ( goGently ) return;
		for ( int v = 0; v < numv; v++ ) {
			they[v].randomize();
		}
		// TODO take poll here, possibly iterate on poll/calcStrategy
		for ( int v = 0; v < numv; v++ ) {
			they[v].calcStrategicPref( poll );
		}
		for ( int sys = 0; sys < nsys; sys++ ) {
			int w;
			systems[sys]->runElection( winners, they );
			w = *winners;
			for ( int v = 0; v < numv; v++ ) {
				they[v].strategySuccess += they[v].getRealPref(w);
			}
		}
	}
}
inline double frand() {
	return (random()/((double)INT_MAX));
}
void NNSVSim::nextg( double keep ) {
	bool* live = new bool[numv];
	int livetarg = (int)(numv * keep);
	int* liveindex = new int[livetarg];
	double sumfit = 0.0;
	double livesumfit = 0.0;
	double minfit = 0.01;
	double dart;
	for ( int v = 0; v < numv; v++ ) {
		live[numv] = false;
		if ( they[v].strategySuccess <= minfit ) {
			they[v].strategySuccess = minfit;
		}
		sumfit += they[v].strategySuccess;
	}
	for ( int i = 0; i < livetarg; i++ ) {
		dart = frand() * sumfit;
		liveindex[i] = 0;
		for ( int v = 0; v < numv; v++ ) if ( !live[v] ) {
			dart -= they[v].strategySuccess;
			if ( dart <= 0.0 ) {
				live[v] = true;
				liveindex[i] = v;
				sumfit -= they[v].strategySuccess;
				livesumfit += they[v].strategySuccess;
				break;
			}
		}
	}
	sumfit = livesumfit;
	for ( int v = 0; v < numv; v++ ) if ( !live[v] ) {
		int pa, pb;
		pa = 0; pb = 0;
		dart = frand() * sumfit;
		for ( int l = 0; l < livetarg; l++ ) {
			dart -= they[liveindex[l]].strategySuccess;
			if ( dart <= 0.0 ) {
				pa = liveindex[l];
				break;
			}
		}
		dart = frand() * sumfit;
		for ( int l = 0; l < livetarg; l++ ) {
			dart -= they[liveindex[l]].strategySuccess;
			if ( dart <= 0.0 ) {
				pb = liveindex[l];
				break;
			}
		}
		they[v].set( they[pa] );
		they[v].mixWith( they[pb] );
		they[v].mutate( 0.1 );
	}
	delete [] live;
	delete [] liveindex;
}

void NNSVSim::print( FILE* f ) {
	for ( int v = 0; v < numv; v++ ) {
		fprintf(f,"%4d: ", v );
		they[v].print( f );
		fprintf(f,"\n");
	}
}
