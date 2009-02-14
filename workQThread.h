#ifndef WORK_QTHREAD_H
#define WORK_QTHREAD_H

class VoterSim;
class ResultFile;
class NameBlock;
class WorkSource;

#include "VoterSim.h"
#include <pthread.h>

class workQThread {
public:
    VoterSim* sim;
    ResultFile* drf;
    NameBlock* nb;
    WorkSource* q;
    pthread_t thread;
    
    workQThread( VoterSim* s, ResultFile* d, NameBlock* n, WorkSource* q );
};
static void* workQThreadProc( void* a ) {
    workQThread* me = (workQThread*)a;
    me->sim->runFromWorkQueue( me->drf, *(me->nb), me->q );
    return NULL;
}
workQThread::workQThread( VoterSim* s, ResultFile* d, NameBlock* n, WorkSource* qIn )
: sim( s ), drf( d ), nb( n ), q( qIn )
{
    pthread_create( &thread, NULL, workQThreadProc, this );
}

#endif
