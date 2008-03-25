#include "ThreadSafeDBRF.h"
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

ThreadSafeDBRF::ThreadSafeDBRF( void* dbIn )
    : DBResultFile( dbIn )
{
    pthread_mutex_init( &m, NULL );
}

ThreadSafeDBRF* ThreadSafeDBRF::open( const char* filename ) {
    ThreadSafeDBRF* toret = NULL;
    DB* f;
    f = dbopen( filename, O_CREAT|O_RDWR, 0777, DB_HASH, NULL );
    if ( f == NULL ) {
	perror("dbopen");
	return NULL;
    }
    toret = new ThreadSafeDBRF( f );
    return toret;
}
Result* ThreadSafeDBRF::get( int choices, int voters, float error ) {
    Result* toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::get( choices, voters, error );
    pthread_mutex_unlock( &m );
    return toret;
}

int ThreadSafeDBRF::put( Result* it, int choices, int voters, float error ) {
    int toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::put( it, choices, voters, error );
    pthread_mutex_unlock( &m );
    return toret;
}

int ThreadSafeDBRF::useNames( const NameBlock* namesIn ) {
    int toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::useNames( namesIn );
    pthread_mutex_unlock( &m );
    return toret;
}

int ThreadSafeDBRF::useStrategyNames( const NameBlock* namesIn ) {
    int toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::useStrategyNames( namesIn );
    pthread_mutex_unlock( &m );
    return toret;
}

int ThreadSafeDBRF::close() {
    int toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::close();
    pthread_mutex_unlock( &m );
    return toret;
}

int ThreadSafeDBRF::flush() {
    int toret;
    pthread_mutex_lock( &m );
    toret = DBResultFile::flush();
    pthread_mutex_unlock( &m );
    return toret;
}

