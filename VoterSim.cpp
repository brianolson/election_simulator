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

extern "C" char* strdup( const char* );

extern volatile int goGently;

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
    : numv( 100 ), numc( 4 ), trials( 10000 ), seats( 1 ), printVoters( false ), 
    printAllResults( false ), text( stdout ), doError( false ),
    confusionError( -1.0 ),
    voterDumpPrefix( NULL ), voterDumpPrefixLen( 0 ),
    voterBinDumpPrefix( NULL ), voterBinDumpPrefixLen( 0 ),
    resultDump( NULL ), resultDumpHtml( NULL ),
    tweValid( false ), summary( basic ),
    resultDumpHtmlVertical( 0 ), winners( NULL ),
    upToTrials( 0 ), strategies( NULL ), numStrat( 0 ),
	rlog(NULL),
	preferenceMode(NSPACE_GAUSSIAN_PREFERENCES),
	dimensions(3)
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
	if ( rlog != NULL ) delete rlog;
}

//char optstring[] = "c:D:d:e:F:H:h:L:l:n:N:qPR:rsS:v:";


int VoterSim::init( int argc, char** argv ) {
    int sys;
	int i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'v':	// number of voters
					++i;
					numv = atoi(argv[i]);
					break;
				case 'n':	// number of trials
					++i;
					trials = atoi(argv[i]);
					break;
				case 'N':	// number of trials
					++i;
					upToTrials = atoi(argv[i]);
					break;
				case 'c':	// number of candidates
					++i;
					numc = atoi(argv[i]);
					break;
				case 'P':	// print all voters for each trial
					printVoters = ! printVoters;
					break;
				case 'r':	// print results for all trails (not just summary)
					printAllResults = ! printAllResults;
					break;
				case 'R':	// result dump
					++i;
					resultDump = fopen( argv[i], "w" );
					if ( resultDump == NULL ) {
						perror( argv[0] );
						fprintf( stderr, "opening result dump file \"%s\" failed\n", argv[i]);
					}
					break;
				case 'H':	// result dump html
					resultDumpHtmlVertical = 1;
				case 'h':	// result dump html
					++i;
					resultDumpHtml = fopen( argv[i], "w" );
					if ( resultDumpHtml == NULL ) {
						perror( argv[0] );
						fprintf( stderr, "opening html result dump file \"%s\" failed\n", argv[i] );
					}
					break;
				case 'e':	// apply error to voter preferences
					doError = true;
					++i;
					confusionError = atof( argv[i] );
					if ( confusionError < 0.0 ) {
						confusionError *= -1.0;
					}
					if ( ! (confusionError <= 2.0) ) {
						confusionError = 2.0;
					}
					break;
				case 'D':	// dump voters, binary
					++i;
					voterBinDumpPrefix = argv[i];
					voterBinDumpPrefixLen = strlen(voterBinDumpPrefix);
					break;
				case 'd':	// dump voters, text
					++i;
					voterDumpPrefix = argv[i];
					voterDumpPrefixLen = strlen(voterDumpPrefix);
					break;
				case 'L': {	// load voters, binary
					++i;
					FILE* fin = fopen( argv[i], "r" );
					if ( !fin ) {
						perror( argv[i] );
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
					++i;
					FILE* fin = fopen( argv[i], "r" );
					if ( !fin ) {
						perror( argv[i] );
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
					++i;
					summary = (printStyle)atoi( argv[i] );
					break;
				case '-': {
					int iret = setLongOpt(argv[i] + 2, argc - (i + 1), argv + (i + 1));
					if (iret < 0) {
						return iret;
					}
					i += iret;
				}
					break;
				default:
					printf("unknown option \'%s\'\n", argv[i] );
					return -1;
			}
		} else {
			printf("unknown option \'%s\'\n", argv[i] );
			return -1;
		}
		++i;
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

int VoterSim::setLongOpt(const char* arg, int argc_after, char** argv_after) {
	if (!strcmp(arg, "nflat")) {
		dimensions = 0;
		preferenceMode = NSPACE_PREFERENCES;
		return 0;
	} else if (!strcmp(arg, "ngauss")) {
		preferenceMode = NSPACE_GAUSSIAN_PREFERENCES;
		return 0;
	} else if (!strcmp(arg, "independentprefs")) {
		preferenceMode = INDEPENDENT_PREFERENCES;
		return 0;
	} else if (!strcmp(arg, "dimensions")) {
		if (argc_after < 1) {
			fprintf(stderr, "missing argument for dimensions");
			return -1;
		}
		char* endp;
		long td = strtol(argv_after[0], &endp, 10);
		if (endp == NULL || endp == argv_after[0]) {
			fprintf(stderr, "failure to parse dimensions argument \"%s\"\n", argv_after[0]);
			return -1;
		}
		dimensions = td;
		return 1;
	} else {
		fprintf(stderr, "bogus arg \"--%s\"\n", arg);
		return -1;
	}
}

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
	seats = wu->seats;
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
	r = drf->get( numc, numv, confusionError, seats );
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
		printResultHeader( stdout, numc, numv, nr->trials, confusionError, seats, nsys, summary );
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
		printResultHeader( stdout, numc, numv, nr->trials, confusionError, seats, nsys, summary );
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
	printResultFooter( stdout, numc, numv, nr->trials, confusionError, seats, nsys, summary );
    }
    if ( drf ) {
	drf->put( nr, numc, numv, confusionError, seats );
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

void VoterSim::randomizeVoters() {
	switch (preferenceMode) {
		case INDEPENDENT_PREFERENCES:
			they.randomize();
			break;
		case NSPACE_PREFERENCES: {
			double* choicePositions = new double[numc*dimensions];
			VoterArray::randomGaussianChoicePositions(choicePositions, numc, dimensions, 0.5);
			they.randomizeNSpace(dimensions, choicePositions, NULL, 1.0, 0.5);
			delete choicePositions;
		}
		case NSPACE_GAUSSIAN_PREFERENCES: {
			double* choicePositions = new double[numc*dimensions];
			VoterArray::randomGaussianChoicePositions(choicePositions, numc, dimensions, 0.5);
			they.randomizeGaussianNSpace(dimensions, choicePositions, NULL, 1.0);
			delete [] choicePositions;
		}
			break;
		default:
			assert(0);
			break;
	}
	if(!they.validate()) {
            fprintf(stderr, "some voter failed validation with all preferences equal, pref mode: %d\n", preferenceMode);
            assert(false);
        }
}

double VoterSim::calculateHappiness(int start, int count, int* winners, double* stddevP, double* giniP) {
	if (seats == 1) {
		if ((stddevP == NULL) && (giniP == NULL)) {
			assert(count == numv);
			return VotingSystem::pickOneHappiness(they, numv, winners[0]);
		} else {
			return VotingSystem::pickOneHappiness(they, count, winners[0], stddevP, giniP, start);
		}
	} else {
		if ((stddevP == NULL) && (giniP == NULL)) {
			assert(count == numv);
			return multiseatHappiness(they, numv, winners, seats);
		} else {
			return multiseatHappiness(they, count, winners, seats, stddevP, giniP, start);
		}
	}
}
