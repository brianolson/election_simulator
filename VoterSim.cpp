#include "VoterSim.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ResultFile.h"
#ifndef NO_DB
#include "DBResultFile.h"
#endif
#include "VotingSystem.h"
#include "WorkQueue.h"

extern "C" char* strdup( const char* );

Strategy::Strategy()
: count( -1 ), next( NULL ),
  happiness( NULL ), happisum( NULL ), ginisum(NULL), happistdsum( NULL ),
  r( NULL )
{}
Strategy::~Strategy(){}
void Strategy::apply( float* dest, const Voter& src ) {
    for ( int i = 0; i < src.numc(); i++ ) {
	dest[i] = src.getPref(i);
    }
}
const char* Strategy::name() {
    static const char* snmae = "No Strategy";
    return snmae;
}
class Maximize : public Strategy {
public:
    virtual void apply( float* dest, const Voter& src );
    virtual const char* name();
    virtual ~Maximize();
};
void Maximize::apply( float* dest, const Voter& src ) {
    for ( int i = 0; i < src.numc(); i++ ) {
	float v;
	v  = src.getPref(i);
	if ( v > 0.0 ) {
	    dest[i] = 1.0;
	} else if ( v < 0.0 ) {
	    dest[i] = -1.0;
	} else {
	    dest[i] = 0.0;
	}
    }
}
const char* Maximize::name() {
    static const char* mnmae = "Maximize";
    return mnmae;
}
Maximize::~Maximize(){}
class Favorite : public Strategy {
public:
    virtual void apply( float* dest, const Voter& src );
    virtual const char* name();
    virtual ~Favorite();
};
void Favorite::apply( float* dest, const Voter& src ) {
    int maxi = src.getMax();
    int i;
    for ( i = 0; i < src.numc(); i++ ) {
	dest[i] = -1.0;
    }
    dest[maxi] = 1.0;
}
const char* Favorite::name() {
    static const char* mnmae = "Favorite";
    return mnmae;
}
Favorite::~Favorite(){}

#if 0
double giniWelfare( const VoterArray& they, int numv, int winner, double* stddevP, int start = 0 ) {
    double avg = 0.0;
    double spreadsum = 0.0;
    int i, j, end;
    end = start + numv;
    for ( i = start; i < end; i++ ) {
	double hi;
	hi = they[i].getPref( winner );
	for ( j = i + 1; j < end; j++ ) {
	    double hj;
	    hj = they[j].getPref( winner );
	    spreadsum += fabs( hi - hj );
	}
	avg += hi;
    }
    avg /= numv;
    return avg - ( spreadsum / ( numv * numv ) );
}
#endif

VoterSim::VoterSim()
    : numv( 100 ), numc( 4 ), trials( 10000 ), printVoters( false ), 
    printAllResults( false ), text( stdout ), doError( false ),
    confusionError( -1.0 ),
    voterDumpPrefix( NULL ), voterDumpPrefixLen( 0 ),
    voterBinDumpPrefix( NULL ), voterBinDumpPrefixLen( 0 ),
    resultDump( NULL ), resultDumpHtml( NULL ),
    tweValid( false ), summary( basic ),
    resultDumpHtmlVertical( 0 ), winners( NULL ),
    upToTrials( 0 ), strategies( NULL ), numStrat( 0 )
{
}
VoterSim::~VoterSim() {
    if ( winners != NULL ) delete [] winners;
    if ( happisum != NULL ) delete [] happisum;
    if ( ginisum != NULL ) delete [] ginisum;
    if ( happistdsum != NULL ) delete [] happistdsum;
    for ( int sys = 0; sys < nsys; sys++ ) {
        delete [] happiness[sys];
    }
    if ( happiness != NULL ) delete [] happiness;
}

char optstring[] = "c:D:d:e:F:H:h:L:l:n:N:qPR:rsS:v:";


int VoterSim::init( int argc, char** argv ) {
    int c;
    int sys;
    extern char* optarg;
    extern int optind;
#if HAVE_OPT_RESET
    extern int optreset;
    
    optreset = 1;
#endif
    optind = 1;
    
    while ( (c = getopt(argc,argv,optstring)) != -1 ) switch ( c ) {
    case 'v':	// number of voters
        numv = atoi( optarg );
        break;
    case 'n':	// number of trials
        trials = atoi( optarg );
        break;
    case 'N':	// number of trials
        upToTrials = atoi( optarg );
        break;
    case 'c':	// number of candidates
        numc = atoi( optarg );
        break;
    case 'P':	// print all voters for each trial
        printVoters = ! printVoters;
        break;
    case 'r':	// print results for all trails (not just summary)
        printAllResults = ! printAllResults;
        break;
    case 'R':	// result dump
	resultDump = fopen( optarg, "w" );
	if ( resultDump == NULL ) {
	    perror( argv[0] );
	    fprintf( stderr, "opening result dump file \"%s\" failed\n", optarg );
	}
	break;
    case 'H':	// result dump html
	resultDumpHtmlVertical = 1;
    case 'h':	// result dump html
	resultDumpHtml = fopen( optarg, "w" );
	if ( resultDumpHtml == NULL ) {
	    perror( argv[0] );
	    fprintf( stderr, "opening html result dump file \"%s\" failed\n", optarg );
	}
	break;
    case 'e':	// apply error to voter preferences
        doError = true;
        confusionError = atof( optarg );
        if ( confusionError < 0.0 ) {
            confusionError *= -1.0;
        }
        if ( ! (confusionError <= 2.0) ) {
            confusionError = 2.0;
        }
        break;
    case 'D':	// dump voters, binary
	voterBinDumpPrefix = optarg;
	voterBinDumpPrefixLen = strlen(voterBinDumpPrefix);
	break;
    case 'd':	// dump voters, text
	voterDumpPrefix = optarg;
	voterDumpPrefixLen = strlen(voterDumpPrefix);
	break;
    case 'L': {	// load voters, binary
	FILE* fin = fopen( optarg, "r" );
	if ( !fin ) {
	    perror( optarg );
	    break;
	}
	fread( &numv, sizeof(numv), 1, fin );
	fread( &numc, sizeof(numc), 1, fin );
	they.build( numv, numc );
	for ( int v = 0; v < numv; v++ ) {
	    they[v].read( fin, numc );
	}
	fclose( fin );
	trials = 1;
    }	break;
    case 'l': {	// load voters, text
	FILE* fin = fopen( optarg, "r" );
	if ( !fin ) {
	    perror( optarg );
	    break;
	}
	fscanf( fin, "%d", &numv );
	fscanf( fin, "%d", &numc );
	they.build( numv, numc );
	for ( int v = 0; v < numv; v++ ) {
	    they[v].undump( fin, numc );
	}
	fclose( fin );
	trials = 1;
    }	break;
    case 'q':	// quiet, no summary
	summary = noPrint;
	break;
    case 's':	// enable strategies
	numStrat = 3;
	strategies = new Strategy*[numStrat];
	strategies[0] = new Maximize();
	strategies[2] = new Favorite();
	strategies[1] = new Strategy();
	break;
    case 'S':	// summary style
	summary = (printStyle)atoi( optarg );
	break;
    default:
	printf("unknown option \'%c\'\n", c );
    }
    
    if ( winners != NULL ) delete [] winners;
    winners = new int[numc*nsys];

    happisum = new double[nsys];
    ginisum = new double[nsys];
    happistdsum = new double[nsys];
    happiness = new double*[nsys];
    for ( sys = 0; sys < nsys; sys++ ) {
        happiness[sys] = new double[trials];
    }
    if ( strategies ) {
	int curcount = 0;
	for ( int i = 0; i < numStrat; i++ ) {
	    if ( strategies[i]->count == -1 ) {
		strategies[i]->count = numv / numStrat;
	    }
	    curcount += strategies[i]->count;
	    strategies[i]->happisum = new double[nsys];
	    strategies[i]->ginisum = new double[nsys];
	    strategies[i]->happistdsum = new double[nsys];
	    strategies[i]->happiness = new double*[nsys];
	    for ( sys = 0; sys < nsys; sys++ ) {
		strategies[i]->happiness[sys] = new double[trials];
	    }
	}
	strategies[numStrat-1]->count += numv - curcount;
    }
    return 0;
}

extern volatile int goGently;

#if 0
u_int32_t voterSteps[] = { 10, 100, 1000, 10000, 100000 };
u_int32_t choiceSteps[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 16, 20, 25, 31, 38, 50, 65, 99 };
float errorSteps[] = { -1, 0, 0.01, 0.10, 0.25, 0.5, 1.0, 1.5, 2.0 };

void VoterSim::runSteps( DBResultFile* drf, NameBlock& nb ) {
    int vstep, cstep, estep;
    for ( vstep = 0; vstep < (int)(sizeof(voterSteps)/sizeof(u_int32_t)); vstep++ ) {
	numv = voterSteps[vstep];
	for ( cstep = 0; cstep < (int)(sizeof(choiceSteps)/sizeof(u_int32_t)); cstep++ ) {
	    numc = choiceSteps[cstep];
	    they.clear();
	    if ( doError || strategies ) {
		theyWithError.clear();
	    }
    	    for ( estep = 0; estep < (int)(sizeof(errorSteps)/sizeof(float)); estep++ ) {
		confusionError = errorSteps[estep];
		run( drf, nb );
		if ( goGently ) return;
	    }
	}
    }
}
#endif

void VoterSim::runFromWorkQueue( ResultFile* drf, NameBlock& nb, WorkSource* q ) {
    WorkUnit* wu;
    while ( (wu = q->newWorkUnit()) != NULL ) {
	numv = wu->numv;
	numc = wu->numc;
	confusionError = wu->error;
	if ( confusionError >= 0 ) {
	    doError = true;
	} else {
	    doError = false;
	}
	trials = wu->trials;
	delete wu;
    if ( winners != NULL ) delete [] winners;
    winners = new int[numc*nsys];
	run( drf, nb );
	if ( goGently ) return;
    }
}

void VoterSim::run( ResultFile* drf, NameBlock& nb ) {
    Result* r = NULL;
    Result* nr = NULL;

    nr = newResult( nsys * (numStrat + 1) );
    if ( drf != NULL ) {
	r = drf->get( numc, numv, confusionError );
	if ( r ) {
	    memcpy( nr, r, sizeofRusult( nsys * (numStrat + 1) ) );
	    free(r);
	}
    }
    if ( upToTrials ) {
	if ( upToTrials <= nr->trials ) {
	    free( nr );
	    return;
	}
	if ( upToTrials < trials + nr->trials ) {
	    trials = upToTrials - nr->trials;
	}
    }
    run( nr );
    if ( goGently ) { fprintf(stderr,"aborted\n");return; }
    if ( summary ) {
#if 01
	if ( strategies ) {
	    char* name = NULL;
	    int namelen;
		const char* splitter;
		int splen;
		static const char textSplitter[] = " | ";
		static const char htmlTableSplitter[] = "</td><td>";
		if ( summary == htmlWithStrategy ) {
			splitter = htmlTableSplitter;
		} else {
			splitter = textSplitter;
		}
		splen = strlen( splitter );
		printResultHeader( stdout, numc, numv, nr->trials, confusionError, nsys, summary );
	    for ( int i = 0; i < nsys; i++ ) {
		name = nb.names ? strdup(nb.names[i]) : strdup("");
		name = (char*)realloc( name, 1024 );
		if ( name == NULL ) {
		    fprintf(stderr,"%s:%d realloc failed\n", __FILE__, __LINE__ );
		    return;
		}
		namelen = strlen( name );
		strcpy( name + namelen, splitter );
		namelen += splen;
		strcpy( name + namelen, "avg" );
		printSystemResult( stdout, nr->systems + i, name, summary );
		for ( int st = 0; st < numStrat; st++ ) {
		    strcpy( name + namelen, strategies[st]->name() );
		    printSystemResult( stdout, nr->systems + ((nsys*(st+1)) + i), name, summary );
		}
	    }
	    if ( name ) free( name );
	} else {
		printResultHeader( stdout, numc, numv, nr->trials, confusionError, nsys, summary );
	    printResult( stdout, nr, nb.names, nsys, summary );
	}
#else
	printResult( stdout, nr, nb.names, nsys, summary );
	if ( strategies ) for ( int st = 0; st < numStrat; st++ ) {
	    if ( summary == 4 ) {
		fprintf( stdout, "<tr><td colspan=\"3\" bgcolor=\"#cccccc\">Strategy \"%s\":</td></tr>\n", strategies[st]->name() );
	    } else if ( summary != 0 ) {
		fprintf( stdout, "strategy \"%s\":\n", strategies[st]->name() );
	    }
	    for ( int i = 0; i < nsys; i++ ) {
		printSystemResult( stdout, nr->systems + ((nsys*(st+1)) + i), nb.names ? nb.names[i] : NULL, summary );
	    }
	}
#endif
	printResultFooter( stdout, numc, numv, nr->trials, confusionError, nsys, summary );
    }
    if ( drf ) {
	drf->put( nr, numc, numv, confusionError );
    }
    free( nr );
}

#if 0
/* This "optimization" of running all the happinesses at once turns out to be slower. */
void pickOneHappinessV( const VoterArray& they, int numv, int numc, int start, double* happiness, double* stddev, double* gini ) {
    int i, end;
    double hi[numc];
    end = start + numv;
    for ( i = start; i < end; i++ ) {
	for ( int c = 0; c < numc; c++ ) {
	    hi[c] = they[i].getPref( c );
	    happiness[c] += hi[c];
	}
	for ( int j = i + 1; j < end; j++ ) {
	    for ( int c = 0; c < numc; c++ ) {
		double hj;
		hj = they[j].getPref( c );
		gini[c] += fabs( hi[c] - hj );
	    }
	}
    }
    // for calculating gini welfare, offset -1..1 happiness to 0..2
    for ( int c = 0; c < numc; c++ ) {
	gini[c] = ( gini[c] / ( numv * (happiness[c]+numv) ) );
	happiness[c] = happiness[c] / numv;
	stddev[c] = 0;
    }
    for ( i = start; i < end; i++ ) {
	double d;
	for ( int c = 0; c < numc; c++ ) {
	    d = they[i].getPref( c ) - happiness[c];
	    stddev[c] += ( d * d );
	}
    }
    for ( int c = 0; c < numc; c++ ) {
	stddev[c] = stddev[c] / numv;
	stddev[c] = sqrt( stddev[c] );
    }
}
#endif

#if 0
void VoterSim::runNoPrintcrap( Result* r ) {
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
    they.build( numv, numc );
    for ( i = 0; i < trials; i++ ) {
	if ( goGently ) return;
	if ( trials != 1 ) {
	    for ( int v = 0; v < numv; v++ ) {
		they[v].randomize();
	    }
	}
	if ( strategies ) {
	    Voter tv(numc);
	    int sti = 0;
	    Strategy* st = strategies[sti++];
	    int stlim = st->count;
            for ( int v = 0; v < numv; v++ ) {
		if ( doError ) {
		    tv.setWithError( they[v], confusionError );
		    theyWithError[v].set( tv, st );
		} else {
		    theyWithError[v].set( they[v], st );
		}
		if ( --stlim == 0 ) {
		    st = strategies[sti++];
		    stlim = st->count;
		}
	    }
	} else if ( doError ) {
            for ( int v = 0; v < numv; v++ ) {
                theyWithError[v].setWithError( they[v], confusionError );
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
	    if ( strategies ) {
		int stpos = 0;
		for ( int st = 0; st < numStrat; st++ ) {
		    strategies[st]->happisum[sys] += strategies[st]->happiness[sys][i] = VotingSystem::pickOneHappiness( they, strategies[st]->count, *winners, &td, stpos );
		    stpos = strategies[st]->count;
		    happistdsum[sys] += td;
		}
	    }
        }
	if ( i+1 < trials ) {
            for ( int v = 0; v < numv; v++ ) {
		they[v].randomize();
	    }
	}
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
    r->trials = ttot;
}
#endif
void VoterSim::run( Result* r ) {
#include "VoterSim_run.h"
}
