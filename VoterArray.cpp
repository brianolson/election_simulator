#include "Voter.h"
#include "gauss.h"
#include <math.h>

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

void VoterArray::randomize() {
	for (int v = 0; v < numv; ++v) {
		they[v].randomize();
	}
}

// place the voters in N-dimensional space, uniformly distributed within a -1..1 N-cube.
// choicePositions is interpreted as 2D, [choice one x, y, z, ...][choice two x, y, z, ...]...
// center can be NULL or double[dimensions] defining the center of the N-cube.
// scale multiplies each position to change the size of the N-cube.
// prefenece for a choice is (approvalDistance - voter_choice_distance)
void VoterArray::randomizeNSpace(int dimensions, double* choicePositions, double* center, double scale, double approvalDistance) {
	double* dimholder = new double[dimensions];
	for (int v = 0; v < numv; ++v) {
		// random position for this voter
		for (int d = 0; d < dimensions; ++d) {
			dimholder[d] = uniformOneOneRandom() * scale;
			if (center != NULL) {
				dimholder[d] += center[d];
			}
		}
		// compare to choices
		for (int c = 0; c < numc; ++c) {
			double rsquared = 0.0;
			double* choiceCoords = choicePositions + (c*dimensions);
			for (int d = 0; d < dimensions; ++d) {
				double x = choiceCoords[d] - dimholder[d];
				rsquared += x * x;
			}
			they[v].setPref(c, approvalDistance - sqrt(rsquared));
		}
	}
	delete dimholder;
}

// place the voters in N-dimensional space, uniformly distributed within a gaussian distribution.
// choicePositions is interpreted as 2D, [choice one x, y, z, ...][choice two x, y, z, ...]...
// center can be NULL or double[dimensions] defining the center of the N-cube.
// prefenece for a choice is (approvalDistance - voter_choice_distance)
void VoterArray::randomizeGaussianNSpace(int dimensions, double* choicePositions, double* center, double sigma) {
	double* dimholder = new double[dimensions];
	struct random_gaussian_context gc = INITIAL_GAUSSIAN_CONTEXT;
	for (int v = 0; v < numv; ++v) {
		// random position for this voter
		for (int d = 0; d < dimensions; ++d) {
			dimholder[d] = random_gaussian_r(&gc) * sigma;
			if (center != NULL) {
				dimholder[d] += center[d];
			}
		}
		// compare to choices
		for (int c = 0; c < numc; ++c) {
			double rsquared = 0.0;
			double* choiceCoords = choicePositions + (c*dimensions);
			for (int d = 0; d < dimensions; ++d) {
				double x = choiceCoords[d] - dimholder[d];
				rsquared += x * x;
			}
			they[v].setPref(c, sigma - sqrt(rsquared));
		}
	}
	delete dimholder;
}

// poisitions is written into, must be allocated double[numc*dimensions]
// static
void VoterArray::randomGaussianChoicePositions(double* positions, int numc, int dimensions, double sigma) {
	struct random_gaussian_context gc = INITIAL_GAUSSIAN_CONTEXT;
	for (int c = 0; c < numc; ++c) {
		double* choiceCoords = positions + (c*dimensions);
		for (int d = 0; d < dimensions; ++d) {
			choiceCoords[d] = random_gaussian_r(&gc) * sigma;
		}
	}
}
