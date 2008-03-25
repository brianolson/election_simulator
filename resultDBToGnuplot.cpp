#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if 0
#include "Voter.h"
#include "VoterSim.h"
#include "VotingSystem.h"
#endif

#include "ResultFile.h"
#include "WorkQueue.h"
#include "DBResultFile.h"

class OutputMode {
public:
    int enabled;
    char* name;
    char* modeString;
    char* dotSuffix;
};
static OutputMode modePng = {1,"png","set terminal png color\nset size 1.5,1.5\n",".png"};
static OutputMode modePs = {0,"ps","set terminal postscript landscape color solid\nset size 1.0,1.0\n",".ps"};
//static OutputMode modePs = {0,"ps","set terminal postscript portrait color solid\nset size 1.0,1.0\n",".ps"};
//   {"set terminal pdf\n",".pdf"},
static OutputMode modeEps = {0,"eps","set terminal postscript eps color solid\nset size 1.0,1.0\n",".eps"};
static OutputMode modeEpsLaTeX = {0,"epsltx","set terminal epslatex color solid\nset size 1.0,1.0\n",".eps"};
static OutputMode modeLaTeX = {0,"ltx","set terminal latex\nset size 1.0,1.0\n",".ltx"};
OutputMode* outputModes[] = {
    &modePng,
    &modePs,
    &modeEps,
    &modeLaTeX,
    &modeEpsLaTeX,
    NULL
};

void setOModeEnabled( char* name, int enabled ) {
    for ( int mi = 0; outputModes[mi] != NULL; mi++ ) {
	if ( !strcmp( name, outputModes[mi]->name ) ) {
	    outputModes[mi]->enabled = enabled;
	}
    }
}

class outset {
public:
    int enabled;
    char* name;
    char* obuf;
    char* ocur;
    inline void ocurUp(void){ while ( *ocur != '\0' ) { ocur++; } }
    virtual void setupText( FILE* GP, int v, int c, float e ) = 0;
    virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) = 0;
    virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) = 0;
    virtual int scan( FILE* GP, int v, int c, float e ) = 0;
    virtual void foreach( FILE* GP, const OutputMode* omode ) = 0;
    
    virtual void byOutset( FILE* GP, const OutputMode* omode, int v, int c, float e );
    virtual ~outset() = 0;
};

#if 01
Steps s( NULL, NULL, NULL, 10 );
#define numCsteps s.cstepsLen
#define numEsteps s.estepsLen
#define numVsteps s.vstepsLen
#define csteps s.csteps
#define vsteps s.vsteps
#define esteps s.esteps
#else
#define MAX_STEPS 100
int csteps[MAX_STEPS];
int vsteps[MAX_STEPS];
float esteps[MAX_STEPS];
int numCsteps = 0, numVsteps = 0, numEsteps = 0;
#endif

// a DBT is small (2 machine words), have a lot of them.
#define MAX_KEYS 8000
//DBT keys[MAX_KEYS];
int numKeys = 0;

inline int present( int v, int* arr, int len ) {
    for ( int i = 0; i < len; i++ ) {
	if ( arr[i] == v ) {
	    return 1;
	}
    }
    return 0;
}
inline int present( float v, float* arr, int len ) {
    for ( int i = 0; i < len; i++ ) {
	if ( arr[i] == v ) {
	    return 1;
	}
    }
    return 0;
}
inline void insert( int v, int* arr, int& len ) {
    int i = len - 1;
    len++;
    for ( ; i >= 0; i-- ) {
	if ( arr[i] > v ) {
	    arr[i+1] = arr[i];
	} else {
	    arr[i+1] = v;
	    return;
	}
    }
    arr[0] = v;
}
inline void insert( float v, float* arr, int& len ) {
    int i = len - 1;
    len++;
    for ( ; i >= 0; i-- ) {
	if ( arr[i] > v ) {
	    arr[i+1] = arr[i];
	} else {
	    arr[i+1] = v;
	    return;
	}
    }
    arr[0] = v;
}

char* outputPrefix = "graph/";
DBResultFile* drf;

const char* renames[] = {
    "Acceptance Vote", "Approval",
    "Ranked Vote", "Borda count",
    "Ranked Vote, no neg pref", "Borda count, truncated",
    "Rated Vote, raw", "Rating Sum, raw",
    "Rated Vote, equal sum", "Rating Sum, normalized",
    "Rated Vote, maximized", "Rating Sum, maximized",
    "Rated Vote, 1..num choices", "Rating Sum, quantized 1..choices",
    "Rated Vote, 1..10", "Rating Sum, quantized 1..10",
    NULL, NULL
};

const char* methodrename( const char* a ) {
    int i = 0;
    while ( renames[i] != NULL ) {
	if ( !strcmp( a, renames[i] ) ) {
	    return renames[i+1];
	}
	i += 2;
    }
    return a;
}

char* printPlotTitles( char* ocur ) {
    sprintf( ocur, "plot \\\n" );
    while ( *ocur != '\0' ) { ocur++; }
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	if ( hi + 1 >= drf->names.nnames ) {
	    sprintf( ocur, "\'-\' title \"%s\"\n", methodrename(drf->names.names[hi]) );
	} else {
	    sprintf( ocur, "\'-\' title \"%s\", \\\n", methodrename(drf->names.names[hi]) );
	}
	while ( *ocur != '\0' ) { ocur++; }
    }
    return ocur;
}
void outset::byOutset( FILE* GP, const OutputMode* omode, int v, int c, float e ) {
    char outputname[256];
    int err;

    obuf = (char*)malloc(1024*8);
    ocur = obuf;
    sprintf( ocur, omode->modeString );
    ocurUp();
    makeOutDirname( outputname, sizeof(outputname), v, c, e );
    err = mkdir( outputname, 0755 );
    if ( err == -1 && errno != EEXIST ) {
	fprintf(stderr,"%s:%d mkdir \"%s\" fails: %s\n", __FILE__, __LINE__, outputname, strerror( errno ) );
	//perror("mkdir");
	return;
    }
    makeOutname( outputname, sizeof(outputname), v, c, e, omode );
    sprintf( ocur, "set output \"%s\"\n", outputname );
    ocurUp();
    setupText( GP, v, c, e );
    ocur = printPlotTitles( ocur );
    err = scan( GP, v, c, e );
    if ( err > 0 ) {
//	printf( "obuf 0x%x ocur 0x%x\n", obuf, ocur );
	fputs( obuf, GP );
    } else {
	printf("skipping %s %d %d %f err=%d\n", outputname, v, c, e, err );
    }
    free( obuf );
    ocur = obuf = NULL;
}
outset::~outset(){};
class outsetvc : public outset {
public:
    virtual void foreach( FILE* GP, const OutputMode* omode ) {
    for ( int vi = 0; vi < numVsteps; vi++ ) {
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    byOutset( GP, omode, vsteps[vi], csteps[ci], -2.0 );
	}
    }
}
    virtual ~outsetvc(){};
};
class outsetve : public outset {
public:
    virtual void foreach( FILE* GP, const OutputMode* omode ) {
    for ( int vi = 0; vi < numVsteps; vi++ ) {
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    byOutset( GP, omode, vsteps[vi], -2, esteps[ei] );
	}
    }
}
    virtual ~outsetve() {};
};
class outsetce : public outset {
public:
    virtual void foreach( FILE* GP, const OutputMode* omode ) {
    for ( int ci = 0; ci < numCsteps; ci++ ) {
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    byOutset( GP, omode, -2, csteps[ci], esteps[ei] );
	}
    }
}
    virtual ~outsetce(){};
};
class vcHappiness : public outsetvc {
public:
    virtual ~vcHappiness(){};
vcHappiness() { name = "e happiness"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set nologscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Potential Error\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Happiness\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"%d Choices, %d Voters\"\n", c, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sv%d/c%02d%s", outputPrefix, v, c, omode->dotSuffix );
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    Result* r;
	    r = drf->get( c, v, esteps[ei] );
	    if ( r ) {
		sprintf( ocur, "%f\t%f\n", (esteps[ei] == -1.00) ? -.1 : esteps[ei], r->systems[hi].meanHappiness );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
#if 0
outset byvcSet = { 1, "e happiness", vcHappinessText, makevcHOutputdirname, makevcHOutputname, vcHappinessScan, vcForeach };
void byvc( FILE* GP, int v, int c, const OutputMode* omode ) {
    byOutset( GP, &byvcSet, omode, v, c, -2.0 );
}
#endif
class vcConsensus : public outsetvc {
public:
vcConsensus() { name = "e consensus"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set nologscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Potential Error\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Consensus (intra-election happiness std-dev, lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"%d Choices, %d Voters\"\n", c, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%scv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%scv%d/c%02d%s", outputPrefix, v, c, omode->dotSuffix );
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    Result* r;
	    r = drf->get( c, v, esteps[ei] );
	    if ( r ) {
		sprintf( ocur, "%f\t%f\n", (esteps[ei] == -1.00) ? -.1 : esteps[ei], r->systems[hi].consensusStdAvg );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};

class vcReliability : public outsetvc {
public:
vcReliability() { name = "e reliability"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set nologscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Potential Error\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Reliability (inter-election happiness std-dev, lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"%d Choices, %d Voters\"\n", c, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%srv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%srv%d/c%02d%s", outputPrefix, v, c, omode->dotSuffix );
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    Result* r;
	    r = drf->get( c, v, esteps[ei] );
	    if ( r ) {
		sprintf( ocur, "%f\t%f\n", (esteps[ei] == -1.00) ? -.1 : esteps[ei], r->systems[hi].reliabilityStd );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
class vcGini : public outsetvc {
public:
    virtual ~vcGini(){};
vcGini() { name = "e gini"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set nologscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Potential Error\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Gini Index (lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"%d Choices, %d Voters\"\n", c, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sgv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sgv%d/c%02d%s", outputPrefix, v, c, omode->dotSuffix );
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    Result* r;
	    r = drf->get( c, v, esteps[ei] );
	    if ( r ) {
		sprintf( ocur, "%f\t%f\n", (esteps[ei] == -1.00) ? -.1 : esteps[ei], r->systems[hi].giniWelfare );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
class vcGiniWelfare : public outsetvc {
public:
    virtual ~vcGiniWelfare(){};
    vcGiniWelfare() { name = "e gini welfare"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set nologscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Potential Error\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Gini Welfare (higher better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"%d Choices, %d Voters\"\n", c, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sgw%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sgw%d/c%02d%s", outputPrefix, v, c, omode->dotSuffix );
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ei = 0; ei < numEsteps; ei++ ) {
	    Result* r;
	    r = drf->get( c, v, esteps[ei] );
	    if ( r ) {
		sprintf( ocur, "%f\t%f\n", (esteps[ei] == -1.00) ? -.1 : esteps[ei], r->systems[hi].meanHappiness * ( 1 - r->systems[hi].giniWelfare ) );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};

class veHappiness : public outsetve {
public:
veHappiness() { name = "c happiness"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale x\n"
	"set nologscale y\n");
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Choices\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Happiness\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Voters\"\n", e, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sv%d/e%0.2f%s", outputPrefix, v, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    Result* r;
	    r = drf->get( csteps[ci], v, e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", csteps[ci], r->systems[hi].meanHappiness );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
#if 0
outset byveSet = { 1, "c happiness", veHappinessText, makeveHOutputdirname, makeveHOutputname, veHappinessScan, veForeach };
void byve( FILE* GP, int v, float e, const OutputMode* omode ) {
    byOutset( GP, &byveSet, omode, v, -2, e );
}
#endif

class veReliability : public outsetve {
public:
veReliability() { name = "c reliability"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale x\n"
	"set nologscale y\n");
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Choices\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Reliability (inter-election happiness std-dev, lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Voters\"\n", e, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%srv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%srv%d/e%0.2f%s", outputPrefix, v, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    Result* r;
	    r = drf->get( csteps[ci], v, e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", csteps[ci], r->systems[hi].reliabilityStd );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
class veConsensus : public outsetve {
public:
veConsensus() { name = "c consensus"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale x\n"
	"set nologscale y\n");
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Choices\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Consensus (intra-election happiness std-dev, lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Voters\"\n", e, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%scv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%scv%d/e%0.2f%s", outputPrefix, v, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    Result* r;
	    r = drf->get( csteps[ci], v, e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", csteps[ci], r->systems[hi].consensusStdAvg );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
class veGini : public outsetve {
public:
veGini() { name = "c gini"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale x\n"
	"set nologscale y\n");
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Choices\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Gini Index (lower better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Voters\"\n", e, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sgv%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sgv%d/e%0.2f%s", outputPrefix, v, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    Result* r;
	    r = drf->get( csteps[ci], v, e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", csteps[ci], r->systems[hi].giniWelfare );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
class veGiniWelfare : public outsetve {
public:
veGiniWelfare() { name = "c gini welfare"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale x\n"
	"set nologscale y\n");
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Choices\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Gini Welfare (higher better)\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Voters\"\n", e, v );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sgw%d", outputPrefix, v );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sgw%d/e%0.2f%s", outputPrefix, v, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int ci = 0; ci < numCsteps; ci++ ) {
	    Result* r;
	    r = drf->get( csteps[ci], v, e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", csteps[ci], r->systems[hi].meanHappiness * ( 1 - r->systems[hi].giniWelfare ) );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};

class ceHappiness : public outsetce {
public:
ceHappiness() { name = "v happiness"; enabled = 1; }
virtual void setupText( FILE* GP, int v, int c, float e ) {
    sprintf( ocur, "set logscale xy\n" );
    ocurUp();
    sprintf( ocur, "set data style linespoints\n" );
    ocurUp();
    sprintf( ocur, "set xlabel \"Number of Voters\"\n" );
    ocurUp();
    sprintf( ocur, "set ylabel \"Happiness\"\n" );
    ocurUp();
    sprintf( ocur, "set title \"Potential Error = %0.2f, %d Choices\"\n", e, c );
    ocurUp();
}
virtual void makeOutDirname( char* outputname, int outputnameLen, int v, int c, float e ) {
    snprintf( outputname, outputnameLen, "%sce", outputPrefix );
}
virtual void makeOutname( char* outputname, int outputnameLen, int v, int c, float e, const OutputMode* omode ) {
    snprintf( outputname, outputnameLen, "%sce/c%02de%0.2f%s", outputPrefix, c, e, omode->dotSuffix );
    *(strchr( outputname, '.' )) = '_';
}
virtual int scan( FILE* GP, int v, int c, float e ) {
    int count = 0;
    for ( int hi = 0; hi < drf->names.nnames; hi++ ) {
	// print all data with v and c, and various e
	for ( int vi = 0; vi < numVsteps; vi++ ) {
	    Result* r;
	    r = drf->get( c, vsteps[vi], e );
	    if ( r ) {
		sprintf( ocur, "%d\t%f\n", vsteps[vi], r->systems[hi].meanHappiness );
		ocurUp();
		count++;
	    }
	    free(r);
	}
	sprintf( ocur, "e\n");
	ocurUp();
    }
    sprintf( ocur, "\n" );
    ocurUp();
    return count;
}
};
#if 0
outset byceSet = { 1, "v happiness", ceHappinessText, makeceHOutputdirname, makeceHOutputname, ceHappinessScan, ceForeach };
void byce( FILE* GP, int c, float e, const OutputMode* omode ) {
    byOutset( GP, &byceSet, omode, -2, c, e );
}
#endif

outset* osets[] = {
    new vcHappiness(),
    new vcConsensus(),
    new vcReliability(),
    new vcGini(),
    new vcGiniWelfare(),
    new veHappiness(),
    new veReliability(),
    new veConsensus(),
    new veGini(),
    new veGiniWelfare(),
    new ceHappiness(),
    NULL
};

void setOutsetEnabled( char* name, int enabled ) {
    for ( int mi = 0; osets[mi] != NULL; mi++ ) {
	if ( !strcmp( name, osets[mi]->name ) ) {
	    osets[mi]->enabled = enabled;
	}
    }
}

static const char* usage = "resultDBtoGnuplot -F resultDB \\\n"
"\t[--esteps esteps][--vsteps vsteps][--csteps csteps] \\\n"
"\t[-m enableFormat][-M disableFormat]\n"
"\t[-o gnuplotCommandFile]\n"
"\t[-s enableSet][-S disableSet]\n"
"\t[--nomode][--noset]\n"
/*"\t\n"
"\t\n"*/
;

void printusage(void){
    printf( usage );
    printf("output formats:\n");
    for ( int mi = 0; outputModes[mi] != NULL; mi++ ) {
	printf("\t\'%s\'\t%s by default\n", outputModes[mi]->name, outputModes[mi]->enabled ? "on" : "off" );
    }
    printf("output sets:\n");
    for ( int mi = 0; osets[mi] != NULL; mi++ ) {
	printf("\t\'%s\'\t%s by default\n", osets[mi]->name, osets[mi]->enabled ? "on" : "off" );
    }
}

int main( int argc, char** argv, char** envp ) {
    char* drfname = NULL;
    char* vstepList = NULL;
    char* cstepList = NULL;
    char* estepList = NULL;
    char* gnuplotCommandsName = "gnuplotCommands";
    for ( int i = 1; i < argc; i++ ) {
	if ( !strcmp( "-F", argv[i] ) ) {
	    i++;
	    drfname = argv[i];
	} else if ( !strcmp( "--esteps", argv[i] ) ) {
	    i++;
	    estepList = argv[i];
	} else if ( !strcmp( "--vsteps", argv[i] ) ) {
	    i++;
	    vstepList = argv[i];
	} else if ( !strcmp( "--csteps", argv[i] ) ) {
	    i++;
	    cstepList = argv[i];
	} else if ( !strcmp( "-m", argv[i] ) ) {
	    i++;
	    setOModeEnabled( argv[i], 1 );
	} else if ( !strcmp( "-M", argv[i] ) ) {
	    i++;
	    setOModeEnabled( argv[i], 0 );
	} else if ( !strcmp( "-o", argv[i] ) ) {
	    i++;
	    gnuplotCommandsName = argv[i];
	} else if ( !strcmp( "-s", argv[i] ) ) {
	    i++;
	    // enable output Set
	    setOutsetEnabled( argv[i], 1 );
	} else if ( !strcmp( "-S", argv[i] ) ) {
	    i++;
	    // disable output Set
	    setOutsetEnabled( argv[i], 0 );
	} else if ( !strcmp( "--nomode", argv[i] ) ) {
	    for ( int mi = 0; outputModes[mi] != NULL; mi++ ) {
		outputModes[mi]->enabled = 0;
	    }
	} else if ( !strcmp( "--noset", argv[i] ) ) {
	    for ( int mi = 0; osets[mi] != NULL; mi++ ) {
		osets[mi]->enabled = 0;
	    }
	} else if ( !(strcmp( "--help", argv[i] ) && strcmp( "-h", argv[i] )) ) {
	    printusage();
	    exit(0);
	} else if ( !strcmp( "", argv[i] ) ) {
	} else {
	    fprintf(stderr,"bogus argument %d \'%s\'\n", i, argv[i] );
	    printusage();
	    exit(1);
	}
    }
    if ( ! drfname ) {
	fprintf(stderr,"no result DB specified\n");
	printusage();
	exit(1);
    }
    drf = DBResultFile::open( drfname, O_RDONLY );
    if ( drf == NULL ) {
	perror("drf");
	exit(1);
    }
    if ( (vstepList == NULL) || (cstepList == NULL) || (estepList == NULL) ) {
	DBT k, d;
#if DB4
	DBC* curs = NULL;
#endif
	int err;
	long long totalElectionSims = 0;
	k.data = NULL;
	k.size = 0;
#if DB4
	k.data = malloc(4096); k.size = 4096; k.ulen = 4096; k.flags = DB_DBT_USERMEM;
	d.data = malloc(4096); d.size = 4096; d.ulen = 4096; d.flags = DB_DBT_USERMEM;
	//d.data = NULL; d.size = 0;
	err = drf->db->cursor( drf->db, NULL, &curs, 0 );
	if ( err != 0 ) {
	    fprintf(stderr,"%s:%d cursor err %d\n", __FILE__, __LINE__, err );
	} else {
	    err = curs->c_get( curs, &k, &d, DB_NEXT );
	    if ( err != 0 ) {
		fprintf(stderr,"%s:%d c_get err %d\n", __FILE__, __LINE__, err );
	    }
	}
#else
	err = drf->db->seq( drf->db, &k, &d, R_FIRST );
#endif
	while ( err == 0 ) {
		u_int32_t tt;
	    ResultKeyStruct rks;
//	    printf("%0.8x\t%0.8x\t", k.data, d.data );
	    if ( k.size != sizeof(rks) ) {
//		printf("bogus k.size %d\n", k.size );
		goto nextRks;
	    }
		memcpy( &rks, k.data, sizeof(rks) );
	    //printf("%0.8x\t%0.8x\n", ((u_int32_t*)k.data)[0], ((u_int32_t*)k.data)[1] );
	    //hexprint( k.data, k.size );
	    if ( ((u_int32_t*)&rks)[0] <= 0x00ffffff ) {
//		Result* r = (Result*)d.data;
//		printResultHeader( stdout, rks->choices, rks->voters, r->trials, rks->error, drf->numSystems() );
//		printResult( stdout, r, drf->names.names, drf->numSystems() );
			memcpy( &tt, &(((Result*)(d.data))->trials), 4 );
			totalElectionSims += tt;
			printf("v %d\tc %d\te %f\ttrials %d %lld\n", rks.voters, rks.choices, rks.error, tt, totalElectionSims );
#if 01
		s.addIfNotPresent( rks.voters, rks.choices, rks.error );
#else
		if ( ! present( rks->choices, csteps, numCsteps ) ) {
		    insert( rks->choices, csteps, numCsteps );
		}
		if ( ! present( rks->voters, vsteps, numVsteps ) ) {
		    insert( rks->voters, vsteps, numVsteps );
		}
		if ( ! present( rks->error, esteps, numEsteps ) ) {
		    insert( rks->error, esteps, numEsteps );
		}
#endif
	//	keys[numKeys] = k;
		numKeys++;
	    } else {
//		printf("%x\n", ((u_int32_t*)&rks)[0] );
	    }
#if 0
	    k.data = NULL;
	    k.size = 0;
#endif
	    nextRks:
#if DB4
	    err = curs->c_get( curs, &k, &d, DB_NEXT );
#else
	    err = drf->db->seq( drf->db, &k, &d, R_NEXT );
#endif
	}
#if DB4
	curs->c_close( curs );
	free(d.data);
#endif
	fprintf(stderr,"db ended with err %d\n",err );
	if ( err == -1 ) {
	    perror("db->get");
	    exit(1);
	}
	printf("totalElectionSims\t%lld\n", totalElectionSims );
    }

    printf("%d keys\n", numKeys);
    if ( vstepList != NULL ) {
	s.parseV( vstepList );
    }
    if ( cstepList != NULL ) {
	s.parseC( cstepList );
    }
    if ( estepList != NULL ) {
	s.parseE( estepList );
    }
    int i;
    printf("vsteps:");
    for ( i = 0; i < numVsteps; i++ ) {
	printf(" %d", vsteps[i] );
    }
    printf("\n");
    printf("csteps:");
    for ( i = 0; i < numCsteps; i++ ) {
	printf(" %d", csteps[i] );
    }
    printf("\n");
    printf("esteps:");
    for ( i = 0; i < numEsteps; i++ ) {
	printf(" %f", esteps[i] );
    }
    printf("\n");
    FILE* GP = fopen( gnuplotCommandsName, "w" );
    for ( int mi = 0; outputModes[mi] != NULL; mi++ ) if ( outputModes[mi]->enabled ) {
	printf("running mode \"%s\"\n", outputModes[mi]->name );
	for ( int oi = 0; osets[oi] != NULL; oi++ ) if ( osets[oi]->enabled ) {
	    printf("running mode \"%s\"\n", osets[oi]->name );
	    osets[oi]->foreach( GP, outputModes[mi] );
	}
    }
    fclose( GP );
    delete drf;
}
