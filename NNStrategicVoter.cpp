#include "NNStrategicVoter.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define numc preflen

inline double frand() {
	return (random()/((double)INT_MAX));
}

void NNStrategicVoter::setRealPref( float* realPrefIn ) {
	memcpy( realPref, realPrefIn, sizeof(float[numc]) );
}
void NNStrategicVoter::calcStrategicPref( double* pollRating ) {
	double hiddenV[hidden];
	int ranking[numc];// [0] index of favorite, [1] index of second, ...
    int node, source;
	int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;
	// sort realPrefs into a ranking
	for ( int i = 0; i < numc; i++ ) {
		ranking[i] = i;
	}
	{
		bool notdone = true;
		while ( notdone ) {
			notdone = false;
			for ( int i = 1; i < numc; i++ ) {
				if ( realPref[ranking[i-1]] < realPref[ranking[i]] ) {
					notdone = true;
					int tr = ranking[i];
					ranking[i] = ranking[i-1];
					ranking[i-1] = tr;
				}
			}
		}
	}
    for ( node = 0; node < hidden; node++) {
		double td;
    	td = 0;
    	for ( source = 0; source < numc; source++ ) {
    	    td += realPref[ranking[source]] * nn[node*histride + source];
    	}
    	for ( source = 0; source < numc; source++ ) {
    	    td += pollRating[ranking[source]] * nn[node*histride + numc + source];
    	}
		// bias input
		td += nn[node*histride + numc*2];
    	//sigmoid response
		//hiddenV[node] = 1 / ( 1 + exp( -td ));
		//sigmoid stretched to -1.0 .. 1.0
    	//hiddenV[node] = (2.0 / ( 1 + exp( -td ))) - 1.0;
		// linear response
		hiddenV[node] = td;
    }
    for ( node = 0; node < numc; node++) {
		double td;
    	td = 0;
    	for ( source = 0; source < hidden; source++ ) {
    	    td += hiddenV[source] * nn[node*ostride + source + hiddenWeightsOffset];
    	}
		// bias input
		td += nn[node*ostride + source + hiddenWeightsOffset];
#if 0
    	//sigmoid response, modified for -1..1 output
    	preference[ranking[node]] = (2.0 / ( 1.0 + exp( -td ))) - 1.0;
#else
		// linear response
		if ( td > 1.0 ) {
			td = 1.0;
		} else if ( td < -1.0 ) {
			td = -1.0;
		} else if ( isnan( td ) ) {
			td = 0.0;
		}
		preference[ranking[node]] = td;
#endif
    }
}

NNStrategicVoter::NNStrategicVoter()
: Voter(), nn( NULL ), hidden( 0 ), realPref( NULL ), strategySuccess( 0.0 )
{
	
}
NNStrategicVoter::NNStrategicVoter( int numcIn )
: Voter( numcIn ) {
	hidden = numc;
}


int NNStrategicVoter::floatCount( int numc, int hidden ) {
	return nnLen( numc, hidden ) + (numc * 2);
}

void NNStrategicVoter::setStorage( float* p, int pl ) {
    preference = p;
    hidden = preflen = pl;
	p += pl;
	realPref = p;
	p += pl;
	nn = p;
}

void NNStrategicVoter::clearStorage() {
    preference = NULL;
    hidden = preflen = 0;
	realPref = NULL;
	nn = NULL;
}

void NNStrategicVoter::randomize() {
#if 01
    for ( int i = 0; i < preflen; i++ ) {
#if 01
        realPref[i] = (frand() - 0.5) * 2.0;
#else
        // scale [0.0 - 1.0) to [-1.0 - 1.0)
        realPref[i] = (drand48() - 0.5) * 2.0;
#endif
    }
#else
	Voter::randomize();
	memcpy( realPref, preference, sizeof(*realPref)*preflen );
#endif
}
void NNStrategicVoter::randomizeNN() {
	int nnl = nnLen( preflen, hidden );
    for ( int i = 0; i < nnl; i++ ) {
#if 01
        nn[i] = (frand() - 0.5) * 2.0;
#else
        // scale [0.0 - 1.0) to [-1.0 - 1.0)
        realPref[i] = (drand48() - 0.5) * 2.0;
#endif
    }
}

int NNStrategicVoter::nnLen( int numc, int hidden ) {
	int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;
	return numc*ostride + hiddenWeightsOffset + 1;
}
void NNStrategicVoter::set( const NNStrategicVoter& it ) {
	memcpy( nn, it.nn, nnLen( numc, hidden ) * sizeof(*nn) );
}
void NNStrategicVoter::mutate( double rate ) {
	int nnmax = nnLen( numc, hidden );
	while ( frand() < rate ) {
		int point;
		point = (int)(frand() * nnmax);
		if ( frand() < 0.7 ) {
			nn[point] *= (frand() - 0.5) * 4.0;
//		} else if ( frand() < 0.8 ) {
//			setHonest();
		} else {
			nn[point] = (frand() - 0.5) * 4.0;
		}
	}
}
void NNStrategicVoter::mixWith( const NNStrategicVoter& it ){
	int nnmax = nnLen( numc, hidden );
	int point;
	point = (int)(frand() * nnmax);
	if ( random() & 1 ) {
		// this gets point and after
		for ( ; point < nnmax; point++ ) {
			nn[point] = it.nn[point];
		}
	} else {
		// this gets before point
		for ( int i = 0; i < point; i++ ) {
			nn[i] = it.nn[i];
		}
	}
}

void NNStrategicVoter::setHonest() {
	int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;

	for ( int h = 0; h < hidden; h++ ) {
		for ( int i = 0; i < histride; i++ ) {
			if ( (h == i) && (h < numc) ) {
				nn[h*histride + i] = 1.0;
			} else {
				nn[h*histride + i] = 0.0;
			}
		}
	}
	for ( int o = 0; o < preflen; o++ ) {
		for ( int h = 0; h < ostride; h++ ) {
			if ( o == h ) {
				nn[hiddenWeightsOffset + ostride*o + h] = 1.0;
			} else {
				nn[hiddenWeightsOffset + ostride*o + h] = 0.0;
			}
		}
	}
}

// "pretty" text
void NNStrategicVoter::printRealPref( void* fi ) const {
    FILE* f = (FILE*)fi;
	/*int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;*/

    fprintf(f,"realPref      ( % .5f", realPref[0]);
    for ( int i = 1; i < preflen; i++ ) {
        fprintf(f," %.5f",realPref[i]);
    }
}
void NNStrategicVoter::printNN( void* fi ) const {
    FILE* f = (FILE*)fi;
	int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;
	
    fprintf(f,"\nnn:");
	for ( int h = 0; h < hidden; h++ ) {
		fprintf(f,"\t% .5f", nn[h*histride]);
		for ( int i = 1; i < histride; i++ ) {
			fprintf(f," % .5f",nn[h*histride + i]);
		}
		fprintf(f,"\n");
	}
	for ( int o = 0; o < preflen; o++ ) {
		fprintf(f,"\t% .5f", nn[hiddenWeightsOffset + ostride*o] );
		for ( int h = 1; h < ostride; h++ ) {
			fprintf(f," % .5f", nn[hiddenWeightsOffset + ostride*o + h] );
		}
		fprintf(f,"\n");
	}
}
void NNStrategicVoter::printStrategicPref( void* fi ) const {
    FILE* f = (FILE*)fi;
	/*int histride = numc*2 + 1;
	int hiddenWeightsOffset = histride * hidden;
	int ostride = hidden + 1;*/
	
    fprintf(f,"strategicPref ( % .5f", preference[0]);
    for ( int i = 1; i < preflen; i++ ) {
        fprintf(f," % .5f",preference[i]);
    }
    fprintf(f," ) fit=%g", strategySuccess );
}
void NNStrategicVoter::print( void* fi ) const {
	printRealPref( fi );
	printNN( fi );
	printStrategicPref( fi );
}
// ugly, parse friendly text
void NNStrategicVoter::dump( void* fi ) const {
    FILE* f = (FILE*)fi;
    fprintf(f,"%.15g", preference[0]);
    for ( int i = 1; i < preflen; i++ ) {
        fprintf(f,"\t%.15g",preference[i]);
    }
    fprintf(f,"\n");
}
// text
void NNStrategicVoter::undump( void* fi, int len ) {
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
void NNStrategicVoter::write( void* fi ) const {
    FILE* f = (FILE*)fi;
    fwrite( preference, sizeof(*preference), preflen, f );
}

// binary
void NNStrategicVoter::read( void* fi, int len ) {
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

#undef numc

NNSVArray::NNSVArray()
: numv( 0 ), numc( 0 ), they( NULL ), storage( NULL )
{
}
NNSVArray::NNSVArray( int nv, int nc ) {
    build( nv, nc );
}
void NNSVArray::build( int nv, int nc ) {
    if ( numv == nv && numc == nc )
		return;
    if ( they )
		clear();
    numv = nv;
    numc = nc;
    they = new NNStrategicVoter[numv];
	int fc = NNStrategicVoter::floatCount(numc,numc);
    storage = new float[numv*fc];
    for ( int i = 0; i < numv; i++ ) {
		they[i].setStorage( storage + (i*fc), numc );
    }
}
NNSVArray::~NNSVArray() {
    clear();
}
void NNSVArray::clear() {
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
