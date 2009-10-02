#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <pthread.h>

class WorkUnit {
public:
    int numv;
    int numc;
    float error;
    int seats;

    int trials;

    WorkUnit* next;

    WorkUnit( int nv, int nc, float e, int t );
    WorkUnit( int nv, int nc, float e, int t, int s );
    
    void print( void* file );
};

class WorkSource {
public:
    virtual WorkUnit* newWorkUnit() = 0;
    virtual ~WorkSource() = 0;
};

class WorkQueue : public WorkSource {
public:
    WorkQueue();
    virtual ~WorkQueue();
    
    void add( WorkUnit* it );
    WorkUnit* pop();
    virtual WorkUnit* newWorkUnit(); // calls pop

protected:
    WorkUnit* head;
    WorkUnit* tail;
};

class ThreadSafeWorkSource : public WorkSource {
public://protected:
    pthread_mutex_t m;
    WorkSource* realSource;
public:
    ThreadSafeWorkSource( WorkSource* source );
    virtual ~ThreadSafeWorkSource();
    virtual WorkUnit* newWorkUnit();
};

class Steps : public WorkSource {
public:
    virtual WorkUnit* newWorkUnit();
//protected:
    int* vsteps;
    int vstepsLen;
    int* csteps;
    int cstepsLen;
    float* esteps;
    int estepsLen;
    int* seatSteps;  // if not NULL, same length as csteps
    // iterator positions within lists
    int v, c, e;
public:
    Steps();
    Steps( const char* vlist, const char* clist, const char* elist, int nIn );
    virtual ~Steps();
    // parse whitespace separated lists of numbers
    void parseV( const char* vlist );
    void parseC( const char* clist );
    void parseE( const char* elist );
    // (Candidates,Seats) stepping is linked
    // whitespace separated list of %d,%d
    void parseCandidatesSeats( const char* slist );
    void addIfNotPresent( int v, int c, float e );
    int n;
};

#endif
