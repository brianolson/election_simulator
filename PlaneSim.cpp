#include "PlaneSim.h"
#include "XYSource.h"
#include "gauss.h"

#include <assert.h>
#include <string.h>
#include <png.h>

void PlaneSim::addCandidateArg( const char* arg ) {
	candroot = new candidatearg( arg, candroot );
	candcount++;
}

void PlaneSim::build( int numv ) {
	candidates = new pos[candcount];
	they.build( numv, candcount );
	accum = new int[px * py * they.numc];
	memset(accum, 0, sizeof(int)*(px * py * they.numc));
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
	
	accum = it.accum;
	isSlave = true;
	
	candroot = it.candroot;
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
	they.randomizeGaussianNSpace(2, candidatePositions, center, sigma);
	delete [] candidatePositions;
}

void PlaneSim::runPixel(VotingSystem* system, int x, int y, double dx, double dy, int* winners) {
	for ( int n = 0; n < electionsPerPixel; n++ ) {
		randomizeVoters( dx, dy, voterSigma );
		if (doCombinatoricExplode) {
			if (combos == NULL) {
				combos = new int[seats*VoterArray::nChooseK(they.numc, seats)];
			}
			exploded.combinatoricExplode(they, seats, combos);
			system->runMultiSeatElection( winners, exploded, seats );
			assert( winners[0] >= 0 );
			assert( winners[0] < exploded.numc );
			int* winningCombo = combos + (seats*winners[0]);
			for (int s = 0; s < seats; ++s) {
				incAccum( x, y, winningCombo[s] );
			}
		} else {
			system->runMultiSeatElection( winners, they, seats );
			assert( winners[0] >= 0 );
			assert( winners[0] < they.numc );
			for (int s = 0; s < seats; ++s) {
				incAccum( x, y, winners[s] );
			}
		}
	}
}

void PlaneSim::run( VotingSystem* system ) {
	int* winners;
	winners = new int[they.numc];
	if (doCombinatoricExplode) {
		assert(combos == NULL);
		combos = new int[seats*VoterArray::nChooseK(they.numc, seats)];
	}
	for ( int y = 0; y < py; y++ ) {
		double dy = yIndexToCoord( y );
		printf("y=%d\n", y);
		for ( int x = 0; x < px; x++ ) {
			double dx = xIndexToCoord( x );
			runPixel(system, x, y, dx, dy, winners);
		}
	}
	delete [] combos;
	combos = NULL;
	delete [] winners;
}

void PlaneSim::runXYSource(VotingSystem* system, XYSource* source) {
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
			runPixel(system, x, y, dx, dy, winners);
		}
	}
	delete [] winners;
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

static const uint8_t candidateColors[] = {
	255, 0, 0,
	0, 255, 0,
	0, 0, 255,
	255, 255, 0,
	255, 0, 255,
	0, 255, 255,
	127, 127, 127,
	255, 255, 255,
};
static int numCandidateColors = sizeof(candidateColors)/3;

png_voidp user_error_ptr = 0;
static void user_error_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "error: %s", str );
}
static void user_warning_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "warning: %s", str );
}

static void writeImageDataToPNGFile( const char* outname, unsigned char** rows, int height, int width, char* args ) {
	FILE* fout;
	
    fout = fopen( outname, "wb");
    if ( fout == NULL ) {
		perror( outname );
		exit(1);
    }
    
    png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING,
		(png_voidp)user_error_ptr, user_error_fn, user_warning_fn );
    if (!png_ptr) {
		fclose(fout);
		exit( 2 );
    }
	
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
		png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
		fclose(fout);
		exit( 2 );
    }
	
    if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fout);
		exit( 2 );
    }
	
    png_init_io(png_ptr, fout);
	
    /* set the zlib compression level */
    png_set_compression_level(png_ptr,
							  Z_BEST_COMPRESSION);
	
    png_set_IHDR(png_ptr, info_ptr, width, height,
				 /*bit_depth*/8,
				 /*color_type*/PNG_COLOR_TYPE_RGB,
				 /*interlace_type*/PNG_INTERLACE_NONE,
				 /*compression_type*/PNG_COMPRESSION_TYPE_DEFAULT,
				 /*filter_method*/PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info(png_ptr, info_ptr);
    png_write_image( png_ptr, rows );

#ifdef PNG_TEXT_SUPPORTED
    if ( args != NULL ) {
	png_text t;
	t.compression = PNG_TEXT_COMPRESSION_zTXt;
	t.key = strdup("spacegraph_args");
	t.text = args;
	t.text_length = strlen( args );
#ifdef PNG_iTXt_SUPPORTED
	t.itxt_length = 0;
	t.lang = 0;
	t.lang_key = 0;
#endif
	png_set_text( png_ptr, info_ptr, &t, 1 );
	free(t.key);
    }
#endif

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose( fout );
}

/* 
. . . b . . .
. . b c b . .
. b c c c b .
b c c c c c b
. b c c c b .
. . b c b . .
. . . b . . .
 */
static uint8_t diamond[] = {
    0,0,0,1,0,0,0,
    0,0,1,2,1,0,0,
    0,1,2,2,2,1,0,
    1,2,2,2,2,2,1,
    0,1,2,2,2,1,0,
    0,0,1,2,1,0,0,
    0,0,0,1,0,0,0,
};
static const int diamondSize = 7;

void PlaneSim::drawDiamond( int cx, int cy, const uint8_t* color ) {
    int half = diamondSize / 2;
    int by = cy - half;
    int bx = cx - half;
    printf("diamond at %d,%d #%02x%02x%02x\n", cx, cy, color[0], color[1], color[2] );
    for ( int y = 0; y < diamondSize; y++ ) {
	if ( by + y < 0 ) {
	    continue;
	}
	if ( by + y >= py ) {
	    break;
	}
	for ( int x = 0; x < diamondSize; x++ ) {
	    if ( bx + x < 0 ) {
		continue;
	    }
	    if ( bx + x >= px ) {
		break;
	    }
	    switch ( diamond[(y*diamondSize) + x] ) {
	    case 0:
		break;
	    case 1: {
		uint8_t* p;
		p = getpxp( bx+x, by+y );
		*p = 0; p++;
		*p = 0; p++;
		*p = 0;
	    }	break;
	    case 2: {
		uint8_t* p;
		p = getpxp( bx+x, by+y );
		p[0] = color[0];
		p[1] = color[1];
		p[2] = color[2];
	    }	break;
	    default:
		assert(0);
	    }
	}
    }
}
void PlaneSim::writePNG( const char* filename ) {
	pix = new uint8_t[px*py*3];
	memset(pix, 0, px*py*3);
	uint8_t** rows = new uint8_t*[py];
	int i;
	int sumErrors = 0;
	double targetSum = electionsPerPixel * seats;
	fprintf(stderr,"making image %dx%d for %d choices\n", px,py, they.numc );
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double sum;
			sum = 0.0;
			for ( i = 0; i < they.numc; i++ ) {
				sum += getAccum( x, y, i );
			}
			if ( sum != targetSum ) {
				fprintf(stderr,"PlaneSim::writePNG error at (%d,%d), sum %f != targetSum %f\n",
					x, y, sum, targetSum );
				sumErrors++;
			}
			for ( i = 0; i < they.numc; i++ ) {
				double weight = getAccum( x, y, i ) / sum;
				const uint8_t* color = candidateColors + ((i % numCandidateColors) * 3);
				uint8_t* p;
				p = getpxp( x, y );
				*p += (uint8_t)(weight * *color);
				p++; color++;
				*p += (uint8_t)(weight * *color);
				p++; color++;
				*p += (uint8_t)(weight * *color);
			}
		}
	}
	for ( i = 0; i < they.numc; i++ ) {
	    printf("cd %f,%f -> ", candidates[i].x, candidates[i].y );
	    drawDiamond( xCoordToIndex( candidates[i].x ),
			 yCoordToIndex( candidates[i].y ),
			 candidateColors + ((i % numCandidateColors) * 3) );
	}
	char* args = new char[1024];
	if ( args != NULL ) {
	    configStr( args, 1024 );
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	rows = NULL;
	delete [] pix;
	pix = NULL;
	if (sumErrors != 0) {
		fprintf(stderr, "%d targetSum errors out of (%dx%d=%d) pixels\n", sumErrors, px, py, px*py);
	}
}
void PlaneSim::writePlanePNG( const char* filename, int c ) {
	pix = new uint8_t[px*py*3];
	memset(pix, 0, px*py*3);
	uint8_t** rows = new uint8_t*[py];
	int i;
	fprintf(stderr,"making image %dx%d for choices %d\n", px, py, c );
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double weight = getAccum( x, y, c );
			weight = weight / electionsPerPixel;
			weight *= 255.0;
			assert(weight <= 255.0);
			assert(weight >= 0.0);
			uint8_t* p;
			p = getpxp( x, y );
			*p = (uint8_t)(weight);
			p++;
			*p = (uint8_t)(weight);
			p++;
			*p = (uint8_t)(weight);
		}
	}
	printf("cd %f,%f -> ", candidates[c].x, candidates[c].y );
	drawDiamond( xCoordToIndex( candidates[c].x ),
				yCoordToIndex( candidates[c].y ),
				candidateColors + ((c % numCandidateColors) * 3) );
	char* args = new char[1024];
	if ( args != NULL ) {
	    configStr( args, 1024 );
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	delete [] pix;
	pix = NULL;
}
void PlaneSim::writeSumPNG( const char* filename ) {
	pix = new uint8_t[px*py*3];
	memset(pix, 0, px*py*3);
	uint8_t** rows = new uint8_t*[py];
	int i;
	double targetSum = electionsPerPixel * seats;
	fprintf(stderr,"making sum image %dx%d\n", px, py );
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double sum;
			sum = 0.0;
			for ( i = 0; i < they.numc; i++ ) {
				sum += getAccum( x, y, i );
			}
			if ( sum > targetSum ) {
				fprintf(stderr,"error at (%d,%d), sum %f > targetSum %f\n",
						x, y, sum, targetSum );
			}
			for ( i = 0; i < they.numc; i++ ) {
				double weight = (sum / targetSum) * 255.0;
				uint8_t* p;
				p = getpxp( x, y );
				*p += (uint8_t)(weight);
				p++;
				*p += (uint8_t)(weight);
				p++;
				*p += (uint8_t)(weight);
			}
		}
	}
	for ( i = 0; i < they.numc; i++ ) {
	    printf("cd %f,%f -> ", candidates[i].x, candidates[i].y );
	    drawDiamond( xCoordToIndex( candidates[i].x ),
					yCoordToIndex( candidates[i].y ),
					candidateColors + ((i % numCandidateColors) * 3) );
	}
	char* args = new char[1024];
	if ( args != NULL ) {
	    configStr( args, 1024 );
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	delete [] pix;
	pix = NULL;
}
void PlaneSim::gaussTest( const char* filename, int nvoters ) {
	int i;
	double dy, dx;
	int x, y;
	printf("generating %d random positions...\n", nvoters );
	for ( i = 0; i < nvoters; i++ ) {
		dy = random_gaussian() * 0.5;
		dx = random_gaussian() * 0.5;
		y = yCoordToIndex( dy );
		x = xCoordToIndex( dx );
		if ( y >= 0 && y < py && x >= 0 && x < px ) {
			incAccum( x, y, 0 );
		}
	}
	printf("stored %d random positions\n", i );
	pix = new uint8_t[px*py*3];
	uint8_t** rows = new uint8_t*[py];
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	int max = 0;
	int myi = -1, mxi = -1;
	for ( y = 0; y < py; y++ ) {
		for ( x = 0; x < px; x++ ) {
			int t = getAccum( x, y, 0 );
			if ( t > max ) {
				max = t;
				myi = y;
				mxi = x;
			}
		}
	}
	printf("max = %d at ( %d, %d )\n", max, mxi, myi );
	for ( y = 0; y < py; y++ ) {
		for ( x = 0; x < px; x++ ) {
			uint8_t value;
			value = 255 - (uint8_t)((255.0 * getAccum(x,y,0)) / max);
			uint8_t* p;
			p = getpxp( x, y );
			*p += value;
			p++;
			*p += value;
			p++;
			*p += value;
		}
	}
	char* args = new char[1024];
	if ( args != NULL ) {
	    configStr( args, 1024 );
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	delete [] pix;
	pix = NULL;
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
	it->sim->runXYSource(it->vs, it->source);
	return NULL;
}
