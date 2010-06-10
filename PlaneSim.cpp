#include "PlaneSim.h"
#include "PlaneSimDraw.h"
#include "XYSource.h"
#include "GaussianRandom.h"
#include "ResultAccumulation.h"
#include "gauss.h"
#if HAVE_PROTOBUF
#include "trial.pb.h"
#endif

#include <assert.h>
#include <string.h>

void PlaneSim::addCandidateArg( const char* arg ) {
	candroot = new candidatearg( arg, candroot );
	candcount++;
}
void PlaneSim::addCandidateArg( double x, double y ) {
	candroot = new candidatearg( x, y, candroot );
	candcount++;
}

void PlaneSim::build( int numv ) {
	candidates = new pos[candcount];
	they.build( numv, candcount );
#if 0
	printf("numc=%d sizeof(pos[numc]) = %lu\n"
		   "px=%d py=%d sizeof(int[px * py * numc]) = %lu\n",
		   numc, sizeof(pos[numc]),
		   px, py, sizeof(int[px * py * numc]) );
#endif
	int i = 0;
	candidatearg* cur = candroot;
	while ( cur != NULL ) {
		candidates[i].x = cur->x;
		candidates[i].y = cur->y;
		fprintf(stderr,"cand[%d] (%f,%f)\n", i, cur->x, cur->y );
		cur = cur->next;
		i++;
	}
	assert( i == candcount );
	assert( i == they.numc );
}

void PlaneSim::coBuild( const PlaneSim& it ) {
	candidates = it.candidates;
	candcount = it.candcount;
	they.build( it.they.numv, candcount );
	minx = it.minx;
	maxx = it.maxx;
	miny = it.miny;
	maxy = it.maxy;
	px = it.px;
	py = it.py;
	voterSigma = it.voterSigma;
	electionsPerPixel = it.electionsPerPixel;
	manhattanDistance = it.manhattanDistance;
	linearFalloff = it.linearFalloff;
	seats = it.seats;
	doCombinatoricExplode = it.doCombinatoricExplode;
	
	systems = it.systems;
	systemsLength = it.systemsLength;
	accum = it.accum;
	isSlave = true;
	
	candroot = it.candroot;

	rootRandom = new BufferDoubleRandomWrapper(it.rootRandom, 512, false);
	gRandom = new GaussianRandom(rootRandom);
}

void PlaneSim::setVotingSystems(VotingSystem** systems_, int numSystems) {
	systems = systems_;
	systemsLength = numSystems;
	assert(accum == NULL);
	accum = new ResultAccumulation*[systemsLength];
	for (int i = 0; i < systemsLength; ++i) {
		accum[i] = new ResultAccumulation(px, py, they.numc);
		accum[i]->clear();
	}
}

void PlaneSim::randomizeVoters( double centerx, double centery, double sigma ) {
	double* candidatePositions = new double[they.numc * 2];
	double center[2] = {
		centerx, centery
	};
	for ( int c = 0; c < they.numc; c++ ) {
		candidatePositions[c*2  ] = candidates[c].x;
		candidatePositions[c*2+1] = candidates[c].y;
	}
	they.randomizeGaussianNSpace(2, candidatePositions, center, sigma,
                                     gRandom);
	delete [] candidatePositions;
}

void PlaneSim::runPixel(int x, int y, double dx, double dy, int* winners) {
	for ( int n = 0; n < electionsPerPixel; n++ ) {
		randomizeVoters( dx, dy, voterSigma );
		for ( int vi = 0; vi < systemsLength; ++vi ) {
			if (doCombinatoricExplode) {
				if (combos == NULL) {
					combos = new int[seats*VoterArray::nChooseK(they.numc, seats)];
				}
				exploded.combinatoricExplode(they, seats, combos);
				systems[vi]->runMultiSeatElection( winners, exploded, seats );
				assert( winners[0] >= 0 );
				assert( winners[0] < exploded.numc );
				int* winningCombo = combos + (seats*winners[0]);
				for (int s = 0; s < seats; ++s) {
					accum[vi]->incAccum( x, y, winningCombo[s] );
				}
			} else {
				systems[vi]->runMultiSeatElection( winners, they, seats );
				assert( winners[0] >= 0 );
				assert( winners[0] < they.numc );
				for (int s = 0; s < seats; ++s) {
					accum[vi]->incAccum( x, y, winners[s] );
				}
			}
		}
	}
}

void PlaneSim::runXYSource(XYSource* source) {
	static const int xySize = 200;
	int xy[xySize*2];
	int xyCount;
	int* winners = new int[they.numc];
	double dx, dy;
	while ((xyCount = source->nextN(xy, xySize)) > 0) {
		for (int i = 0; i < xyCount; ++i) {
			int x = xy[i*2    ];
			int y = xy[i*2 + 1];
			dy = yIndexToCoord( y );
			dx = xIndexToCoord( x );
			runPixel(x, y, dx, dy, winners);
		}
	}
	delete [] winners;
}

#if HAVE_PROTOBUF
Result2** PlaneSim::runRandomXY(Result2** resultsOut) {
	//Result2** resultsOut = new Result2*[systemsLength];
	// TODO: lift this alloc/free
	int* winners = new int[they.numc];
	double x = rootRandom->get() * (maxx - minx) + minx;
	double y = rootRandom->get() * (maxy - miny) + miny;
	randomizeVoters(x, y, voterSigma);
	for ( int vi = 0; vi < systemsLength; ++vi ) {
		Result2* out;
		if (resultsOut[vi] == NULL) {
			out = new Result2();
			resultsOut[vi] = out;
		} else {
			out = resultsOut[vi];
			out->Clear();
		}
		out->add_coords(x);
		out->add_coords(y);
		out->set_system(vi);
		int* twinners;
		if (doCombinatoricExplode) {
			if (combos == NULL) {
				combos = new int[seats*VoterArray::nChooseK(they.numc, seats)];
			}
			exploded.combinatoricExplode(they, seats, combos);
			systems[vi]->runMultiSeatElection( winners, exploded, seats );
			assert( winners[0] >= 0 );
			assert( winners[0] < exploded.numc );
			twinners = combos + (seats*winners[0]);
		} else {
			systems[vi]->runMultiSeatElection( winners, they, seats );
			assert( winners[0] >= 0 );
			assert( winners[0] < they.numc );
			twinners = winners;
		}
		for (int s = 0; s < seats; ++s) {
			out->add_winners(twinners[s]);
		}
		double td, tg, th;
		calculateHappiness( twinners, &th, &td, &tg );
		out->set_mean_happiness(th);
		out->set_voter_happiness_stddev(td);
		out->set_gini_index(tg);
	}
	delete [] winners;
	return resultsOut;
}
#endif

void PlaneSim::calculateHappiness(int* winners, double* happinessP, double* stddevP, double* giniP) {
	if (seats == 1) {
		if ((stddevP == NULL) && (giniP == NULL)) {
			*happinessP = VotingSystem::pickOneHappiness(they, they.numv, winners[0]);
		} else {
			*happinessP = VotingSystem::pickOneHappiness(they, they.numv, winners[0], stddevP, giniP, 0);
		}
	} else {
		if ((stddevP == NULL) && (giniP == NULL)) {
			*happinessP = multiseatHappiness(they, they.numv, winners, seats);
		} else {
			*happinessP = multiseatHappiness(they, they.numv, winners, seats, stddevP, giniP, 0);
		}
	}
}

size_t PlaneSim::configStr( char* dest, size_t len ) {
    size_t toret = 0;
    size_t delta;
    char* outpos;
    toret = snprintf( dest, len, "-minx %f -maxx %f -miny %f -maxy %f -px %d -py %d -Z %f",
	    minx, maxx, miny, maxy, px, py, voterSigma );
    assert( toret > 0 );
    candidatearg* cur = candroot;
    while ( (cur != NULL) && (toret < len) ) {
		outpos = dest + toret;
		delta = snprintf( outpos, len - toret, " -c %f,%f", cur->x, cur->y );
		assert( delta > 0 );
		toret += delta;
		cur = cur->next;
    }
    return toret;
}

PlaneSim::candidatearg::candidatearg( const char* arg, candidatearg* nextI ) : next( nextI ) {
	char* endp = NULL;
	x = strtod( arg, &endp );
	if ( endp == arg || endp == NULL ) {
		fprintf(stderr,"bogus candidate position arg, \"%s\" not a valid number\n", arg );
		exit(1);
	}
	arg = endp + 1;
	y = strtod( arg, &endp );
	if ( endp == arg || endp == NULL ) {
		fprintf(stderr,"bogus candidate position arg, \"%s\" not a valid number\n", arg );
		exit(1);
	}
}

void* runPlaneSimThread(void* arg) {
	PlaneSimThread* it = (PlaneSimThread*)arg;
	it->sim->runXYSource(it->source);
	return NULL;
}

#if HAVE_PROTOBUF
#include "MessageLiteWriter.h"

void* runRandomTestThread(void* arg) {
	PlaneSimThread* it = (PlaneSimThread*)arg;
	Result2** resultsOut = new Result2*[it->sim->systemsLength];
	assert(resultsOut);
	for (int i = 0; i < it->sim->systemsLength; ++i) {
		resultsOut[i] = NULL;
	}
	while (*(it->countToDo) > 0) {
		bool ok = true;
		pthread_mutex_lock(it->countLock);
		if (*(it->countToDo) > 0) {
			*(it->countToDo) = *(it->countToDo) - 1;
		} else {
			ok = false;
		}
		pthread_mutex_unlock(it->countLock);
		if (ok) {
			it->sim->runRandomXY(resultsOut);
			pthread_mutex_lock(it->writerLock);
			for (int i = 0; i < it->sim->systemsLength; ++i) {
				it->writer->writeMessage(resultsOut[i]);
			}
			pthread_mutex_unlock(it->writerLock);
		}
	}
	for (int i = 0; i < it->sim->systemsLength; ++i) {
		delete resultsOut[i];
	}
	delete resultsOut;
	return NULL;
}
#endif /* HAVE_PROTOBUF */
