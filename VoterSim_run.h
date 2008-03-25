/* a naked body of a function, define things, #include this */
int i;
int sys;
if ( r == NULL ) {
    return;
}
for ( sys = 0; sys < nsys; sys++ ) {
    happisum[sys] = 0;
    ginisum[sys] = 0;
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
	strategies[st]->ginisum[sys] = 0;
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
		if ( goGently ) return;
		if ( tweValid ) {
			systems[sys]->runElection( winners + (sys*numc), theyWithError );
		} else {
			systems[sys]->runElection( winners + (sys*numc), they );
		}
		happiness[sys][i] = NAN;
	}
	for ( int osys = 0; osys < nsys; osys++ ) {
		int sys;
		int winner = winners[osys*numc/*+0*/];
		if ( isnan(happiness[osys][i]) ) {
			double td, tg, th;
			th = VotingSystem::pickOneHappiness( they, numv, winner, &td, &tg, 0 );
			sys = osys;
			happisum[sys] += happiness[sys][i] = th;
			happistdsum[sys] += td;
			ginisum[sys] += tg;
			for ( sys = osys+1; sys < nsys; sys++ ) {
				if ( winners[sys*numc] == winner ) {
					happisum[sys] += happiness[sys][i] = th;
					happistdsum[sys] += td;
					ginisum[sys] += tg;
				}
			}
		}
		sys = osys;
		if ( printAllResults ) {
			fprintf(text,"%s\thappiness\t%f\twinner\t%d\n", systems[sys]->name,happiness[sys][i],winner);
			//                fprintf(text,"");
		}
		if ( resultDump != NULL ) {
			fprintf( resultDump, "%.15g\t%d\t", happiness[sys][i], winner );
		}
		if ( resultDumpHtml != NULL ) {
			if ( resultDumpHtmlVertical ) {
				fprintf( resultDumpHtml, "<tr ALIGN=\"center\"><th>%s</th><td>%.15g</td><td>%d</td></tr>\n", systems[sys]->name, happiness[sys][i], winner );
			} else {
				fprintf( resultDumpHtml, "<td>%.15g</td><td>%d</td>", happiness[sys][i], winner );
			}
		}
    }
#if (!defined(strategies)) || strategies
	if ( strategies ) {
		double td, tg;
		for ( sys = 0; sys < nsys; sys++ ) {
			int stpos = 0;
			for ( int st = 0; st < numStrat; st++ ) {
				strategies[st]->happisum[sys] += strategies[st]->happiness[sys][i] = VotingSystem::pickOneHappiness( they, strategies[st]->count, winners[sys*numc/*+0*/], &td, &tg, stpos );
				stpos += strategies[st]->count;
				strategies[st]->happistdsum[sys] += td;
				strategies[st]->ginisum[sys] += tg;
			}
		}
	}
#endif
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
    r->systems[sys].giniWelfare = (ginisum[sys] + r->systems[sys].giniWelfare * r->trials) / ttot;
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
	r->systems[rsys+sys].giniWelfare = (strategies[st]->ginisum[sys] + r->systems[rsys+sys].giniWelfare * r->trials) / ttot;
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
