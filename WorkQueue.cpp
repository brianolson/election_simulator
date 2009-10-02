#include "WorkQueue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WorkUnit::WorkUnit( int nv, int nc, float e, int t )
    : numv( nv ), numc( nc ), error( e ), seats( 1 ),
      trials( t ), next( (WorkUnit*)0 )
{}
WorkUnit::WorkUnit( int nv, int nc, float e, int t, int s )
    : numv( nv ), numc( nc ), error( e ), seats( s ),
      trials( t ), next( (WorkUnit*)0 )
{}
void WorkUnit::print( void* file ) {
    FILE* f = (FILE*)file;
    if (seats > 1) {
        fprintf( f, "%d voters, %d choices, %0.2f error, %d seats, %d trials\n", numv, numc, error, seats, trials );
    } else {
        fprintf( f, "%d voters, %d choices, %0.2f error, %d trials\n", numv, numc, error, trials );
    }
}

WorkSource::~WorkSource(){}

WorkQueue::WorkQueue()
    : head( NULL ), tail( NULL )
{}
WorkQueue::~WorkQueue() {
    WorkUnit* cur = head;
    while ( cur ) {
	WorkUnit* t;
	t = cur->next;
	delete cur;
	cur = t;
    }
}

void WorkQueue::add( WorkUnit* it ) {
    it->next = NULL;
    if ( tail == NULL ) {
	head = tail = it;
    } else {
	tail->next = it;
	tail = it;
    }
}

WorkUnit* WorkQueue::pop() {
    if ( head == NULL ) {
	return NULL;
    } else {
	WorkUnit* toret = head;
	head = head->next;
	if ( head == NULL ) tail = NULL;
	return toret;
    }
}

WorkUnit* WorkQueue::newWorkUnit() {
    return pop();
}

Steps::Steps()
    : seatSteps( NULL ), v( 0 ), c( 0 ), e( -1 )
{
    static const int dvsteps[] = { 100, 1000 };
    static const int dcsteps[] = { 2, 3, 4, 5, 6, 7, 8 };
    static const float desteps[] = { -1, 0.00, 0.01, 0.1, 0.25, 0.5, 1.0, 1.5, 2.0 };
    vsteps = (int*)malloc( sizeof(dvsteps) );
    if ( vsteps == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return;
    }
    memcpy( vsteps, dvsteps, sizeof(dvsteps) );
    vstepsLen = 2;
    csteps = (int*)malloc( sizeof(dcsteps) );
    if ( csteps == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return;
    }
    memcpy( csteps, dcsteps, sizeof(dcsteps) );
    cstepsLen = 7;
    esteps = (float*)malloc( sizeof(desteps) );
    if ( esteps == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return;
    }
    memcpy( esteps, desteps, sizeof(desteps) );
    estepsLen = 9;
    n = 10000;
}
Steps::~Steps() {
    free( vsteps );
    free( csteps );
    free( esteps );
}

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

Steps::Steps( const char* vlist, const char* clist, const char* elist, int nIn )
    : vstepsLen( 1 ), cstepsLen( 1 ), estepsLen( 1 ), seatSteps( NULL ),
      v( 0 ), c( 0 ), e( -1 ), n( nIn )
{
    if ( vlist ) {
	parseV( vlist );
    } else {
	vsteps = (int*)malloc( sizeof(int) );
	vstepsLen = 0;
    }
    if ( clist ) {
	parseC( clist );
    } else {
	csteps = (int*)malloc( sizeof(int) );
	cstepsLen = 0;
    }
    if ( elist ) {
	parseE( elist );
    } else {
	esteps = (float*)malloc( sizeof(float) );
	estepsLen = 0;
    }
}

inline int countNonSpaceChunks(const char* cur) {
    int chunks = 0;
    int mode = 0;  // 0 = space, 1 = chunk
    while ( *cur != '\0' ) {
	if ( isspace( *cur )) {
            if ( mode == 1 ) {
                // nonspace -> space
                mode = 0;
            }
        } else {
            if ( mode == 0 ) {
                // space -> nonspace transition
                chunks++;
                mode = 1;
            }
        }
        cur++;
    }
    return chunks;
}

void Steps::parseV( const char* vlist ) {
    const char* cur;
    int pos;
    vstepsLen = countNonSpaceChunks(vlist);
    vsteps = (int*)malloc( sizeof(int) * vstepsLen );
    cur = vlist;
    pos = 0;
    ((int*)vsteps)[pos] = atoi( cur );
    while ( *cur != '\0' ) {
	if ( isspace( *cur )) {
	    pos++;
	    do {
		cur++;
	    } while ( isspace( *cur ));
	    ((int*)vsteps)[pos] = atoi( cur );
	}
	cur++;
    }
}

void Steps::parseC( const char* clist ) {
    const char* cur;
    int pos;
    cstepsLen = countNonSpaceChunks(clist);
    csteps = (int*)malloc( sizeof(int) * cstepsLen );
    cur = clist;
    pos = 0;
    ((int*)csteps)[pos] = atoi( cur );
    while ( *cur != '\0' ) {
	if ( isspace( *cur )) {
	    pos++;
	    do {
		cur++;
	    } while ( isspace( *cur ));
	    ((int*)csteps)[pos] = atoi( cur );
	}
	cur++;
    }
}

void Steps::parseE( const char* elist ) {
    const char* cur;
    int pos;
    estepsLen = countNonSpaceChunks(elist);
    esteps = (float*)malloc( sizeof(int) * estepsLen );
    cur = elist;
    pos = 0;
    ((float*)esteps)[pos] = atof( cur );
    while ( *cur != '\0' ) {
	if ( isspace( *cur )) {
	    pos++;
	    do {
		cur++;
	    } while ( isspace( *cur ));
	    ((float*)esteps)[pos] = atof( cur );
	}
	cur++;
    }
}

// whitespace separated list of %d,%d
void Steps::parseCandidatesSeats( const char* slist ) {
    const char* cur;
    char* eptr;
    int pos;
    cstepsLen = countNonSpaceChunks(slist);
    assert(cstepsLen > 0);
    csteps = (int*)malloc( sizeof(int) * cstepsLen );
    assert(csteps != NULL);
    seatSteps = (int*)malloc( sizeof(int) * cstepsLen );
    assert(seatSteps != NULL);
    cur = slist;
    pos = 0;
    while ( *cur != '\0' ) {
        while ( (*cur != '\0') && isspace( *cur )) {
            cur++;
        }
        if (*cur == '\0') {
            break;
        }
        assert(pos < cstepsLen);
        csteps[pos] = strtol(cur, &eptr, 10);
        // TODO: make nicer error messages than assert
        assert(eptr != NULL);
        assert(eptr != cur);
        assert(*eptr == ',');
        cur = eptr + 1;
        seatSteps[pos] = strtol(cur, &eptr, 10);
        assert(eptr != NULL);
        assert(eptr != cur);
        cur = eptr;
        pos++;
    }
}

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
void Steps::addIfNotPresent( int v, int c, float e ) {
    if ( ! present( v, vsteps, vstepsLen ) ) {
	vsteps = (int*)realloc( vsteps, (vstepsLen + 1) * sizeof(int) );
	assert( vsteps );
	insert( v, vsteps, vstepsLen );
    }
    if ( ! present( c, csteps, cstepsLen ) ) {
	csteps = (int*)realloc( csteps, (cstepsLen + 1) * sizeof(int) );
	assert( csteps );
	insert( c, csteps, cstepsLen );
    }
    if ( ! present( e, esteps, estepsLen ) ) {
	esteps = (float*)realloc( esteps, (estepsLen + 1) * sizeof(float) );
	assert( esteps );
	insert( e, esteps, estepsLen );
    }
}

WorkUnit* Steps::newWorkUnit() {
    e++;
    if ( e == estepsLen ) {
	e = 0;
	c++;
    }
    if ( c == cstepsLen ) {
	c = 0;
	v++;
    }
    if ( v == vstepsLen ) {
//	v = 0;
	return NULL;
    }
    if ( seatSteps != NULL ) {
      return new WorkUnit( vsteps[v], csteps[c], esteps[e], n, seatSteps[c] );
    }
    return new WorkUnit( vsteps[v], csteps[c], esteps[e], n );
}

#if STEPS_TEST_MAIN
int main( int argc, char** argv ) {
    Steps* s;
    s = new Steps( argv[1], argv[2], argv[3], 1000 );
    WorkUnit* wu;
    while ( (wu = s->newWorkUnit()) != NULL ) {
	wu->print( stdout );
	delete wu;
    }
}
#endif

ThreadSafeWorkSource::ThreadSafeWorkSource( WorkSource* source )
    : realSource( source )
{
    pthread_mutex_init( &m, NULL );
}
ThreadSafeWorkSource::~ThreadSafeWorkSource() {
    pthread_mutex_destroy( &m );
}

WorkUnit* ThreadSafeWorkSource::newWorkUnit() {
    WorkUnit* toret;
    pthread_mutex_lock( &m );
    toret = realSource->newWorkUnit();
    pthread_mutex_unlock( &m );
    return toret;
}
