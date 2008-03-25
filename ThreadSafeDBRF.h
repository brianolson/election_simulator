#ifndef THREAD_SAFE_DBRF_H
#define THREAD_SAFE_DBRF_H

#include "DBResultFile.h"
#include <pthread.h>

class ThreadSafeDBRF : public DBResultFile {
public:
    ThreadSafeDBRF( void* dbIn );
    static ThreadSafeDBRF* open( const char* filename );
    virtual Result* get( int choices, int voters, float error );
    virtual int put( Result* it, int choices, int voters, float error );
    virtual int useNames( const NameBlock* namesIn );
    virtual int useStrategyNames( const NameBlock* namesIn );
    virtual int close();
    virtual int flush();
    
    pthread_mutex_t m;
};


#endif
