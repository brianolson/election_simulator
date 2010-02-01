#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

#include "Voter.h"
#include "VoterSim.h"
#include "VotingSystem.h"

#include "AcceptanceVotePickOne.h"
#include "Condorcet.h"
#include "FuzzyVotePickOne.h"
#include "gauss.h"
#include "InstantRunoffVotePickOne.h"
#include "IRNR.h"
#include "IteratedNormalizedRatings.h"
#include "OneVotePickOne.h"
#include "RandomElection.h"
#include "RankedVotePickOne.h"
#include "Bucklin.h"

#include "WorkQueue.h"

extern "C" char* strdup( const char* );

void Voter::setWithError( const Voter& it, float error ) {
    for ( int i = 0; i < preflen; i++ ) {
        preference[i] = it.preference[i] + ((random() / ((double)LONG_MAX)) - 0.5) * 2.0 * error;
#if 0
        if ( preference[i] < -1.0 ) {
            preference[i] = -1.0;
        } else if ( preference[i] > 1.0 ) {
            preference[i] = 1.0;
        }
#endif
    }
}
void Voter::set( const Voter& it, Strategy* s ) {
    if ( s ) {
	s->apply( preference, it );
    } else {
	for ( int i = 0; i < it.preflen; i++ ) {
	    preference[i] = it.preference[i];
	}
    }
}

// "pretty" text
void Voter::print( void* fi ) const {
    FILE* f = (FILE*)fi;
    fprintf(f,"( %g", preference[0]);
    for ( int i = 1; i < preflen; i++ ) {
        fprintf(f,", %g",preference[i]);
    }
    fprintf(f," )");
}
// ugly, parse friendly text
void Voter::dump( void* fi ) const {
    FILE* f = (FILE*)fi;
    fprintf(f,"%.15g", preference[0]);
    for ( int i = 1; i < preflen; i++ ) {
        fprintf(f,"\t%.15g",preference[i]);
    }
    fprintf(f,"\n");
}
// text
void Voter::undump( void* fi, int len ) {
    FILE* f = (FILE*)fi;
    if ( len != preflen || preference == NULL ) {
	if ( preference ) {
	    delete [] preference;
	}
	preference = new float[len];
	preflen = len;
    }
    for ( int i = 0; i < len; i++ ) {
	fscanf( f, "%f",&(preference[i]));
    }
}
// binary
void Voter::write( void* fi ) const {
    FILE* f = (FILE*)fi;
    fwrite( preference, sizeof(*preference), preflen, f );
}

// binary
void Voter::read( void* fi, int len ) {
    FILE* f = (FILE*)fi;
    if ( len != preflen || preference == NULL ) {
	if ( preference ) {
	    delete [] preference;
	}
	preference = new float[len];
	preflen = len;
    }
    fread( preference, sizeof(*preference), len, f );
}

int Voter::getMax() const {
    int i;
    float m = preference[0];
    int toret = 0;
    
    for ( i = 1; i < preflen; i++ ) {
        if ( preference[i] > m ) {
            m = preference[i];
            toret = i;
        }
    }
    return toret;
}

int Voter::getMin() const {
    int i;
    float m = preference[0];
    int toret = 0;
    
    for ( i = 1; i < preflen; i++ ) {
        if ( preference[i] < m ) {
            m = preference[i];
            toret = i;
        }
    }
    return toret;
}

void Voter::getTopN(int* indecies, float* prefs, int seats) {
	indecies[0] = 0;
	prefs[0] = preference[0];
	int c;
	for (c = 1; c < seats; ++c) {
		float p = preference[c];
		int ci = c;
		while (((ci - 1) >= 0) && (p > prefs[ci - 1])) {
			prefs[ci] = prefs[ci - 1];
			indecies[ci] = indecies[ci - 1];
			ci--;
		}
		prefs[ci] = p;
		indecies[ci] = c;
	}
	for (; c < preflen; ++c) {
		int ci = seats;
		float p = preference[c];
		while (((ci - 1) >= 0) && (p > prefs[ci - 1])) {
			if (ci < seats) {
				prefs[ci] = prefs[ci - 1];
				indecies[ci] = indecies[ci - 1];
			}
			ci--;
		}
		if (ci < seats) {
			prefs[ci] = p;
			indecies[ci] = c;
		}
	}
#ifndef NDEBUG
	for (int c = 0; c < seats - 1; ++c) {
		assert(prefs[c] > prefs[c+1]);
	}
#endif
}

void Voter::getSortedPrefs(int* indecies, float* prefs, const int* choices, int seats) {
	for (int ci = 0; ci < seats; ++ci) {
		indecies[ci] = choices[ci];
		prefs[ci] = preference[choices[ci]];
	}
	// bubble sort!
	bool notdone = true;
	while (notdone) {
		notdone = false;
		for (int i = 0; i < seats - 1; ++i) {
			if (prefs[i+1] > prefs[i]) {
				int ti = indecies[i];
				float tp = prefs[i];
				indecies[i] = indecies[i+1];
				prefs[i] = prefs[i+1];
				indecies[i+1] = ti;
				prefs[i+1] = tp;
				notdone = true;
			}
		}
	}
}

Voter::Voter()
    : preference( NULL ), preflen( 0 ) {
}

Voter::Voter( int numCandidates )
    : preflen( numCandidates ) {
    preference = new float[preflen];
}
// -1.0 .. 1.0
double* prefBias;
inline double biasedRandom( double bias ) {
    // [-1.0 .. 1.0] ==> [Pi/2 .. 0]
    bias = (1.0 - bias) * (M_PI_4);
    // random in [0.0 .. 1.0] biased by exponentiation to same range, expanded to [-1.0 .. 1.0]
    return (pow( (random() / ((double)LONG_MAX)), tan( bias ) ) - 0.5) * 2.0;
}

void Voter::randomize() {
    for ( int i = 0; i < preflen; i++ ) {
		preference[i] = uniformOneOneRandom();
    }
}

void Voter::randomizeWithFirstChoice( int first ) {
    int i;
    for ( i = 0; i < preflen; i++ ) {
		preference[i] = uniformOneOneRandom();
    }
    // sort highest value into position of first choice
    for ( i = 0; i < preflen; i++ ) if ( i != first ) {
	if ( preference[i] > preference[first] ) {
	    float t = preference[i];
	    preference[i] = preference[first];
	    preference[first] = t;
	}
    }
}

// random position within -1..1 N-cube
// static
void Voter::randomUniformCoord(float* coord, int dimensions) {
	for (int i = 0; i < dimensions; ++i) {
		coord[i] = uniformOneOneRandom();
	}
}
// static
// random gaussian position centered at 0
void Voter::randomGaussianCoord(float* coord, int dimensions, double sigma) {
	struct random_gaussian_context gc = INITIAL_GAUSSIAN_CONTEXT;
	for (int i = 0; i < dimensions; ++i) {
		coord[i] = random_gaussian_r(&gc);
	}
}

static bool incrementCombo(int* outcounter, int seats, int numc) {
	int pos = seats - 1;
	if (pos < 0) {
		return false;
	}
	outcounter[pos]++;
	while (outcounter[pos] >= numc) {
		if (pos == 0) return false;
		bool ok = incrementCombo(outcounter, seats - 1, numc);
		if (!ok) return false;
		outcounter[pos] = outcounter[pos - 1] + 1;
	}
	return true;
}

void VoterArray::combinatoricExplode(const VoterArray& source, int seats, int* combos) {
	int newc = nChooseK(source.numc, seats);
	build(source.numv, newc);
	int* outcounter = new int[seats];
	int i;
	int combopos = 0;
	for (i = 0; i < seats; i++) {
		outcounter[i] = i;
	}
	bool ok = true;
	while (ok) {
		for (i = 0; i < seats; ++i) {
			combos[(combopos * seats) + i] = outcounter[i];
		}
		for (int v = 0; v < numv; ++v) {
			double sum = 0.0;
			for (i = 0; i < seats; ++i) {
				sum += source[v].getPref(outcounter[i]);
			}
			they[v].setPref(combopos, sum);
		}
		ok = incrementCombo(outcounter, seats, source.numc);
		combopos++;
	}
	assert(combopos == newc);
	delete [] outcounter;
}

Voter::~Voter() {
    if ( preference ) {
	delete preference;
    }
}

void Voter::setStorage( float* p, int pl ) {
    preference = p;
    preflen = pl;
}

void Voter::clearStorage() {
    preference = NULL;
    preflen = 0;
}

/* v in -1.0 to 1.0 */
double quantize( double v, int steps ) {
    steps--;
    v = v + 1.0;
    v = v / 2.0;
    v = v * steps;
#ifdef __linux__
    v = rint(v);
#else
    v = round(v);
#endif
    v = v / ((double)steps);
    v = v * 2.0;
    v = v - 1.0;
    return v;
}

void VotingSystem::init( const char** envp ) {
	if ( envp == NULL ) {
		return;
	}
	while ( *envp != NULL ) {
		if ( !strcmp( *envp, "-name" ) ) {
			envp++;
			name = *envp;
		} else {
			fprintf( stderr, "unused voting system arg \"%s\"\n", *envp );
		}
		envp++;
	}
}

bool VotingSystem::runMultiSeatElection(int* winnerArray, const VoterArray& they, int seats) const {
	if (seats == 1) {
		runElection(winnerArray, they);
		return true;
	}
	return false;
}

/*
Gini?
  f_ave = sum ( w_i, i=1..n ) / n

  f_Gini = f_ave - sum ( |w_i-w_j|, i=1..n, j=1..n ) / n^2 / 2

The Gini welfare function can also be expressed as

  f_Gini = f_ave * (1-G)

where G is the "Gini coefficient of inequality":

       sum ( |w_i-w_j|, i=1..n, j=1..n )
  G = -----------------------------------
          2 * n * sum ( w_i, i=1..n )


http://en.wikipedia.org/wiki/Gini_coefficient
says:
G = abs( 1 - sum{k,1,n}((X_k - X_{k-})(Y_k + Y_{k-1})) )

I think that requires sorting the population from poorest to richest.

*/


static inline double clampOne(double x) {
  if (x < -1.0) {
    return -1.0;
  }
  if (x > 1.0) {
    return 1.0;
  }
  return x;
}
static inline double oneMultiseatHappiness(const Voter& it, int* winners, int seats) {
  double hi = 0.0;
  for (int s = 0; s < seats; ++s) {
    hi += it.getPref( winners[s] );
  }
  return clampOne(hi);
}

double multiseatHappiness( const VoterArray& they, int numv, int* winners, int seats, double* stddevP, double* giniP, int start ) {
  double happiness = 0.0;
  double spreadsum = 0.0;
  int i, end;
  end = start + numv;
  for ( i = start; i < end; i++ ) {
    double hi = oneMultiseatHappiness(they[i], winners, seats);
    happiness += hi;
    for ( int j = i + 1; j < end; j++ ) {
      double hj = oneMultiseatHappiness(they[j], winners, seats);
      spreadsum += fabs( hi - hj );
    }
  }
  // for calculating gini welfare, offset -1..1 happiness to 0..2
  *giniP = ( spreadsum / ( numv * (happiness+numv) ) );
  happiness = happiness / numv;
  double stddev = 0;
  for ( i = start; i < end; i++ ) {
    double d = oneMultiseatHappiness(they[i], winners, seats) - happiness;
    stddev += ( d * d );
  }
  stddev = stddev / numv;
  stddev = sqrt( stddev );
  *stddevP = stddev;
  return happiness;
}
double multiseatHappiness( const VoterArray& they, int numv, int* winners, int seats ) {
  double happiness = 0.0;
  for ( int i = 0; i < numv; i++ ) {
    double hi = oneMultiseatHappiness(they[i], winners, seats);
    happiness += hi;
  }
  happiness = happiness / numv;
  return happiness;
}
double VotingSystem::pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP, double* giniP, int start ) {
    double happiness = 0.0;
    double spreadsum = 0.0;
    int i, end;
    end = start + numv;
    for ( i = start; i < end; i++ ) {
		double hi;
        hi = they[i].getPref( winner );
		happiness += hi;
	    for ( int j = i + 1; j < end; j++ ) {
			double hj;
			hj = they[j].getPref( winner );
			spreadsum += fabs( hi - hj );
	    }
    }
	// for calculating gini welfare, offset -1..1 happiness to 0..2
	*giniP = ( spreadsum / ( numv * (happiness+numv) ) );
    happiness = happiness / numv;
	double stddev = 0;
	for ( i = start; i < end; i++ ) {
	    double d;
	    d = they[i].getPref( winner ) - happiness;
	    stddev += ( d * d );
	}
	stddev = stddev / numv;
	stddev = sqrt( stddev );
	*stddevP = stddev;
    return happiness;
}
double VotingSystem::pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP, int start ) {
    double happiness = 0;
    int i, end;
    end = start + numv;
    for ( i = start; i < end; i++ ) {
        happiness += they[i].getPref( winner );
    }
    happiness = happiness / numv;
    if ( stddevP != NULL ) {
	double stddev = 0;
	for ( i = start; i < end; i++ ) {
	    double d;
	    d = they[i].getPref( winner ) - happiness;
	    stddev += ( d * d );
	}
	stddev = stddev / numv;
	stddev = sqrt( stddev );
	*stddevP = stddev;
    }
    return happiness;
}
double VotingSystem::pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP ) {
    return pickOneHappiness( they, numv, winner, stddevP, 0 );
}
double VotingSystem::pickOneHappiness( const VoterArray& they, int numv, int winner ) {
    double happiness = 0;
    int i;
    for ( i = 0; i < numv; i++ ) {
        happiness += they[i].getPref( winner );
    }
    happiness = happiness / numv;
    return happiness;
}
VotingSystem::~VotingSystem(){}

class MaxHappiness : public VotingSystem {
public:
	MaxHappiness() : VotingSystem("Max Happiness") {};
	virtual void runElection( int* winnerR, const VoterArray& they ) const {
		double maxh;
		int c = 0;
		int numc = they.numc;
		int numv = they.numv;
		
		maxh = pickOneHappiness( they, numv, c );
		for ( int i = 1; i < numc; i++ ) {
			double h;
			h = pickOneHappiness( they, numv, i );
			if ( h > maxh ) {
				c = i;
				maxh = h;
			}
		}
		if ( winnerR ) *winnerR = c;
	};
	virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	virtual ~MaxHappiness(){};
	double miniMultiHappiness( const VoterArray& they, int* winners, int seats ) const;
};
double MaxHappiness::miniMultiHappiness( const VoterArray& they, int* winners, int seats ) const {
  // Like multiseatHappiness, but doesn't clamp output
  double happiness = 0.0;
  for ( int i = 0; i < they.numv; i++ ) {
    double hi = 0.0;
    for (int s = 0; s < seats; ++s) {
      hi += they[i].getPref( winners[s] );
    }
    happiness += hi;
  }
  happiness = happiness / (they.numv * seats);
  return happiness;
}
bool MaxHappiness::runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const {
	int* testWinners = new int[seats];	// increments over combinations
	int* bestWinners = new int[seats];
	for (int s = 0; s < seats; ++s) {
		testWinners[s] = s;
		bestWinners[s] = s;
	}
	double maxh;
	maxh = miniMultiHappiness( they, testWinners, seats );
	while (incrementCombo(testWinners, seats, they.numc)) {
#ifndef NDEBUG
		for (int i = 0; i < seats; ++i) {
			for (int j = i + 1; j < seats; ++j) {
				assert(testWinners[i] != testWinners[j]);
			}
		}
#endif
		double th;
		th = miniMultiHappiness( they, testWinners, seats );
		if (th > maxh) {
			for (int s = 0; s < seats; ++s) {
				bestWinners[s] = testWinners[s];
			}
			maxh = th;
		}
	}
	for (int s = 0; s < seats; ++s) {
		winnerArray[s] = bestWinners[s];
	}
	delete [] testWinners;
	delete [] bestWinners;
	return true;
}

VotingSystem* newMaxHappiness( const char* n ) {
	return new MaxHappiness();
}
VSFactory* MaxHappiness_f = new VSFactory( newMaxHappiness, "Max" );

VSFactory* VSFactory::root = NULL;

VSFactory::VSFactory( VotingSystem* (*f)( const char* ), const char* n )
: fact( f ), name( n )
{
    next = root;
    root = this;
}

const VSFactory* VSFactory::byName( const char* n ) {
    VSFactory* cur = root;
    while ( cur != NULL ) {
		if ( ! strcmp( n, cur->name ) ) {
			return cur;
		}
		cur = cur->next;
    }
    return NULL;
}

VSConfig* VSConfig::newVSConfig( const VSFactory* f, const char** a, VSConfig* n ) {
    if ( f == NULL ) {
		return n;
    }
    VSConfig* toret = new VSConfig( f, a, n );
    return toret;
}

VSConfig* VSConfig::defaultList( const char** a, VSConfig* n ) {
    VSConfig* toret = n;
#if 01
	toret = new VSConfig( new MaxHappiness(), toret );
	toret = new VSConfig( new OneVotePickOne(), toret );
	toret = new VSConfig( new InstantRunoffVotePickOne(), toret );
	toret = new VSConfig( new AcceptanceVotePickOne(), toret );
	toret = new VSConfig( new FuzzyVotePickOne( "Raw Rating Summation", 0, 0 ), toret );
	toret = new VSConfig( new FuzzyVotePickOne( "Normalized Rating Summation", 1, 0 ), toret );
	toret = new VSConfig( new FuzzyVotePickOne( "Maximized Rating Summation", 2, 0 ), toret );
	toret = new VSConfig( new FuzzyVotePickOne( "Rating Summation, 1..num choices", 0, -1 ), toret );
	toret = new VSConfig( new FuzzyVotePickOne( "Rating Summation, 1..10", 0, 10 ), toret );
	toret = new VSConfig( new RankedVotePickOne(), toret );
	toret = new VSConfig( new RankedVotePickOne("Borda, truncated",1), toret );
	toret = new VSConfig( new Condorcet(), toret );
	toret = new VSConfig( new IRNR("Instant Runoff Normalized Ratings"), toret );
	toret = new VSConfig( new IRNR("Instant Runoff Normalized Ratings, positive shifted", 1.0), toret );
	toret = new VSConfig( new IteratedNormalizedRatings("Iterated Normalized Ratings"), toret );
	toret = new VSConfig( new RandomElection("Random"), toret );
	toret = new VSConfig( new Bucklin(), toret );
#else
	VSFactory* cur = VSFactory::root;
    while ( cur != NULL ) {
		toret = new VSConfig( cur, a, toret );
		cur = cur->next;
    }
#endif
    return toret;
}

VSConfig::~VSConfig() {
    if ( next ) delete next;
    if ( args ) delete args;
}

void VSConfig::init() {
    me = fact->make();
    if ( args && *args ) {
		me->init( args );
    }
}

void VSConfig::print( void* vf ) {
    FILE* f = (FILE*)vf;
    fprintf( f, "fatory \"%s\", ", fact->name );
    if ( args != NULL ) {
		fprintf( f, "args \"%s", args[0] );
		for ( int i = 1; args[i] != NULL; i++ ) {
			fprintf( f, "\", \"%s", args[i] );
		}
		fprintf( f, "\"" );
    } else {
		fprintf( f, "no args" );
    }
    fprintf( f, "\n" );
}

VSConfig* VSConfig::reverseList( VSConfig* v ) {
	VSConfig* tn = next;
	next = v;
	if ( tn == NULL ) {
		return this;
	} else {
		return tn->reverseList( this );
	}
}

/*
 Format:
 arg
 arg
 arg
 !systemName
 ...
 */
//systemList = systemsFromDescFile( argv[i], methodArgs, MAX_METHOD_ARGS, systemList );
VSConfig* systemsFromDescFile( const char* filename, const char** methodArgs, int maxargc, VSConfig* systemList ) {
	FILE* f;
	char lbuf[128];
	char* line;
	int methodArgc = 0;

	f = fopen( filename, "r" );
	if ( f == NULL ) {
		perror( filename );
		return systemList;
	}
	while ( (line = fgets( lbuf, sizeof(lbuf), f )) ) {
		{ char* cr; cr = strchr( line, '\n' ); if ( cr ) *cr = '\0'; }
		if ( line[0] == '!' ) {
			line++;
			// escape, do enable
			if ( line[0] == '!' ) {
				// unescape, do '!'
				goto doarg;
			} else {
				if ( methodArgc > 0 ) {
					const char** marg = new const char*[methodArgc+1];
					for ( int j = 0; j < methodArgc; j++ ) {
						marg[j] = methodArgs[j];
					}
					marg[methodArgc] = NULL;
					systemList = VSConfig::newVSConfig( line, marg, systemList );
				} else {
					systemList = VSConfig::newVSConfig( line, NULL, systemList );
				}
				methodArgc = 0;
			}
		} else {
doarg:
			if ( methodArgc < maxargc ) {
				methodArgs[methodArgc] = strdup(line);
				methodArgc++;
			} else {
				fprintf( stderr, "arg \"%s\" Is beyond limit of %d method args\n", line, maxargc );
			}
		}
	}
	fclose( f );
	return systemList;
}

void voterDump( char* voteDumpFilename, const VoterArray& they, int numv, int numc ) {
    FILE* voteDumpFile;
    voteDumpFile = fopen( voteDumpFilename, "w" );
    if ( voteDumpFile == NULL ) {
	perror( voteDumpFilename );
	return;
    }
    voterDump( voteDumpFile, they, numv, numc );
    fclose( voteDumpFile );
}
void voterDump( FILE* voteDumpFile, const VoterArray& they, int numv, int numc ) {
    fprintf( voteDumpFile, "%d\n%d\n", numv, numc );
    for ( int v = 0; v < numv; v++ ) {
	they[v].dump( voteDumpFile );
    }
	fflush( voteDumpFile );
}
void voterBinDump( char* voteBinDumpFilename, const VoterArray& they, int numv, int numc ) {
    FILE* voteDumpFile;
    voteDumpFile = fopen( voteBinDumpFilename, "w" );
    fwrite( &numv, sizeof(numv), 1, voteDumpFile );
    fwrite( &(numc), sizeof(numc), 1, voteDumpFile );
    for ( int v = 0; v < numv; v++ ) {
	they[v].write( voteDumpFile );
    }
    fclose( voteDumpFile );
}

// inline function to avoid double evaluation of a or b
inline double fmax( double a, double b ) {
    return (a>b)?a:b;
}
