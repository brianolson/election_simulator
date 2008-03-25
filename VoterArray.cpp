#include "Voter.h"

#ifndef NULL
#define NULL 0
#endif

VoterArray::VoterArray()
    : numv( 0 ), numc( 0 ), they( NULL ), storage( NULL )
{
}
VoterArray::VoterArray( int nv, int nc ) {
    build( nv, nc );
}
void VoterArray::build( int nv, int nc ) {
    if ( numv == nv && numc == nc )
	return;
    if ( they )
	clear();
    numv = nv;
    numc = nc;
    they = new Voter[numv];
    storage = new float[numv*numc];
    for ( int i = 0; i < numv; i++ ) {
	they[i].setStorage( storage + (i*numc), numc );
    }
}
VoterArray::~VoterArray() {
    clear();
}
void VoterArray::clear() {
    if ( they ) {
	for ( int i = 0; i < numv; i++ ) {
	    they[i].clearStorage();
	}
	delete [] they;
	they = NULL;
	delete [] storage;
	storage = NULL;
	numv = 0;
	numc = 0;
    }
}
