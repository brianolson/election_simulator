#ifndef WORK_QTHREAD_H
#define WORK_QTHREAD_H

class VoterSim;
class DBResultFile;
class NameBlock;
class WorkSource;

#include "VoterSim.h"
#include <pthread.h>

class workQThread {
public:
    VoterSim* sim;
    DBResultFile* drf;
    NameBlock* nb;
    WorkSource* q;
    pthread_t thread;
    
    workQThread( VoterSim* s, DBResultFile* d, NameBlock* n, WorkSource* q );
};
static void* workQThreadProc( void* a ) {
    workQThread* me = (workQThread*)a;
    me->sim->runFromWorkQueue( me->drf, *(me->nb), me->q );
    return NULL;
}
workQThread::workQThread( VoterSim* s, DBResultFile* d, NameBlock* n, WorkSource* qIn )
: sim( s ), drf( d ), nb( n ), q( qIn )
{
    pthread_create( &thread, NULL, workQThreadProc, this );
}

#endif
