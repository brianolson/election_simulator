#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>

#include "Voter.h"
#include "VoterSim.h"
#include "VotingSystem.h"

#include "OneVotePickOne.h"
#include "RankedVotePickOne.h"
#include "AcceptanceVotePickOne.h"
#include "FuzzyVotePickOne.h"
#include "InstantRunoffVotePickOne.h"
#include "Condorcet.h"
#include "IRNR.h"
#include "IteratedNormalizedRatings.h"
#include "RandomElection.h"

#include "gauss.h"

#include "png.h"

class pos {
public:
	double x, y;
};

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

class candidatearg {
public:
	double x,y;
	candidatearg* next;
	candidatearg( const char* arg, candidatearg* next );
};
candidatearg::candidatearg( const char* arg, candidatearg* nextI ) : next( nextI ) {
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

class PlaneSim {
public:
	pos* candidates;
	VoterArray they;
	
	double minx, maxx, miny, maxy;
	int px, py;
	double voterSigma;
	int electionsPerPixel;

	bool manhattanDistance;
	bool linearFalloff;

	int* accum;
	// indicates that certain pointers aren't owned and should not be freed
	bool isSlave;

	candidatearg* candroot;
	int candcount;
	
	PlaneSim() : candidates( NULL ), minx( -2.0 ), maxx( 2.0 ), miny( -2.0 ), maxy( 2.0 ),
		px( 500 ), py( 500 ), voterSigma( 0.5 ), electionsPerPixel( 10 ),
		manhattanDistance( false ), linearFalloff( false ), accum( NULL ), isSlave( false ),
		candroot( NULL ), candcount( 0 ), pix( NULL )
	{}
	
	void build( int numv );
	// build this to be a slave to it.
	void coBuild( const PlaneSim& it );
	
	void randomizeVoters( double centerx, double centery, double sigma );
	
	void run( VotingSystem* system );
	// calculates first through last inclusive.
	// first and last are ((y*px) + x) indecies.
	// (first >= 0) && (last < px*py)
	void runRange( VotingSystem* system, int first, int last );

	size_t configStr( char* dest, size_t len );

	inline double xIndexToCoord( int x ) {
		double dx;
		dx = (maxx - minx)/(px - 1);
		return minx + dx*x;
	}
	inline double yIndexToCoord( int y ) {
		double dy;
		dy = (maxy - miny)/(py - 1);
		return miny + dy*y;
	}
	inline int getAccum( int x, int y, int c ) const {
		return accum[c + x*they.numc + y*they.numc*px];
	}
	inline void incAccum( int x, int y, int c ) {
		accum[c + x*they.numc + y*they.numc*px]++;
	}
	inline void setAccum( int x, int y, int c, int v ) {
		accum[c + x*they.numc + y*they.numc*px] = v;
	}
	
	inline int xCoordToIndex( double x ) {
		double dx;
		dx = (maxx - minx)/(px - 1);
		return (int)floor( (x - minx) / dx );
	}
	inline int yCoordToIndex( double y ) {
		double dy;
		dy = (maxy - miny)/(py - 1);
		return (int)floor( (y - miny) / dy );
	}

	inline void addCandidateArg( const char* arg ) {
	    candroot = new candidatearg( arg, candroot );
	    candcount++;
	}
	uint8_t* pix;
	
	inline uint8_t* getpxp( int x, int y ) {
		return pix + (x*3 + y*3*px);
	}
	inline void setpx( int x, int y, const uint8_t* color ) {
		int index = px*3*y + 3*x;
		uint8_t* p = pix + index;
		*p = *color;
		p++;color++;
		*p = *color;
		p++;color++;
		*p = *color;
	}
	void gaussTest( const char* filename, int nvoters );
	void writePNG( const char* filename );

	void drawDiamond( int x, int y, const uint8_t* color );
};

void PlaneSim::build( int numv ) {
	candidates = new pos[candcount];
	they.build( numv, candcount );
	accum = new int[px * py * they.numc];
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
	
	accum = it.accum;
	isSlave = true;
	
	candroot = it.candroot;
}

void PlaneSim::randomizeVoters( double centerx, double centery, double sigma ) {
#if 1
	double* candidatePositions = new double[they.numc * 2];
	double center[2] = {
		centerx, centery
	};
	for ( int c = 0; c < they.numc; c++ ) {
		candidatePositions[c*2  ] = candidates[c].x;
		candidatePositions[c*2+1] = candidates[c].y;
	}
	they.randomizeGaussianNSpace(2, candidatePositions, center, sigma);
	delete candidatePositions;
#else
	for ( int i = 0; i < they.numv; i++ ) {
		double tx, ty;
		tx = (random_gaussian() * sigma) + centerx;
		ty = (random_gaussian() * sigma) + centery;
		for ( int c = 0; c < they.numc; c++ ) {
			double dx, dy, r;
			dx = tx - candidates[c].x;
			dy = ty - candidates[c].y;
			if ( manhattanDistance ) {
				r = fabs(dx) + fabs(dy);
			} else {
				r = sqrt( dx * dx + dy * dy );
			}
			if ( linearFalloff ) {
				they[i].setPref( c, sigma - r );
			} else {
				they[i].setPref( c, 1/r );
			}
		}
	}
#endif
}

void PlaneSim::run( VotingSystem* system ) {
	int* winners;
	winners = new int[they.numc];
	for ( int y = 0; y < py; y++ ) {
		double dy = yIndexToCoord( y );
		printf("y=%d\n", y);
		for ( int x = 0; x < px; x++ ) {
			double dx = xIndexToCoord( x );
			for ( int n = 0; n < electionsPerPixel; n++ ) {
				randomizeVoters( dx, dy, voterSigma );
				system->runElection( winners, they );
				assert( winners[0] >= 0 );
				assert( winners[0] < they.numc );
				incAccum( x, y, winners[0] );
			}
			//printf("%d,%d\n", x, y);
		}
	}
	delete [] winners;
}

void PlaneSim::runRange( VotingSystem* system, int first, int last ) {
	int* winners;
	int x, y;
	int pos = first;
	x = first % px;
	y = first / px;
	assert( ((y*px)+x) == first );
	winners = new int[they.numc];
	double dy = yIndexToCoord( y );
	double dx = xIndexToCoord( x );
	while ( true ) {
		//printf("y=%d\n", y);
		for ( int n = 0; n < electionsPerPixel; n++ ) {
			randomizeVoters( dx, dy, voterSigma );
			system->runElection( winners, they );
			assert( winners[0] >= 0 );
			assert( winners[0] < they.numc );
			incAccum( x, y, winners[0] );
		}
		pos++;
		if ( pos > last ) {
			break;
		}
		x++;
		if ( x == px ) {
			x = 0;
			y++;
			assert(y < py);
			dy = yIndexToCoord( y );
		}
		dx = xIndexToCoord( x );
	}
	delete [] winners;
}

size_t PlaneSim::configStr( char* dest, size_t len ) {
    size_t toret = 0;
    size_t delta;
    char* outpos;
    /* PlaneSim() : candidates( NULL ), minx( -2.0 ), maxx( 2.0 ), miny( -2.0 ), maxy( 2.0 ),
		px( 500 ), py( 500 ), voterSigma( 0.5 ), accum( NULL ),
		candroot( NULL ), candcount( 0 ), pix( NULL )
		*/
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

png_voidp user_error_ptr = 0;
void user_error_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "error: %s", str );
}
void user_warning_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "warning: %s", str );
}

void myDoPNG( const char* outname, unsigned char** rows, int height, int width, char* args ) {
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
	uint8_t** rows = new uint8_t*[py];
	int i;
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
			if ( sum != electionsPerPixel ) {
				fprintf(stderr,"error at (%d,%d), sum %f != electionsPerPixel %d\n",
					x, y, sum, electionsPerPixel );
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
	myDoPNG( filename, rows, px, py, args );
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
		//printf("d (%f,%f) => (%d,%d)\n", dx,dy, x,y);
		if ( y >= 0 && y < py && x >= 0 && x < px ) {
			incAccum( x, y, 0 );
		}
#if 0
		if ( i % (px*py/10) == 0 ) {
			printf("%d ", i );
		}
#endif
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
	myDoPNG( filename, rows, px, py, args );
	delete [] rows;
	delete [] pix;
	pix = NULL;
}

volatile int goGently = 0;

void mysigint( int a ) {
    goGently = 1;
}

class PlaneSimThread {
public:
	PlaneSim* sim;
	pthread_t thread;
	VotingSystem* vs;
	int first;
	int last;
};
void* runPlaneSimThread(void* arg) {
	PlaneSimThread* it = (PlaneSimThread*)arg;
	it->sim->runRange( it->vs, it->first, it->last );
	return NULL;
}

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 64
#endif

static void printEMList(void) {
	VSFactory* cf;
	cf = VSFactory::root;
	while ( cf != NULL ) {
		printf("%s\n", cf->name );
		cf = cf->next;
	}
}

const char* usage =
"usage: spacegraph [-o foo.png][-tg][-minx f][-miny f][-maxx f][-maxy f]\n"
     "\t[-px i][-py i][-v voters][-n iter per pix][-Z sigma]\n"
     "\t[-c \"candidateX Y\"][--list][--method electionmethod]\n"
;

#ifndef MAX_METHOD_ENV
#define MAX_METHOD_ENV 20
#endif
const char* methodEnv[MAX_METHOD_ENV] = { 0 };
int methodEnvCount = 0;
char defaultFoutname[] = "tsg.png";

int main( int argc, char** argv ) {
	// trivial main
	VotingSystem* vs = NULL;
	PlaneSim sim;
	int i;
	char* foutname = defaultFoutname;
	char* testgauss = NULL;
	int nvoters = 1000;
	int nthreads = 1;

	for ( i = 1; i < argc; i++ ) {
		if ( ! strcmp( argv[i], "-o" ) ) {
			i++;
			foutname = argv[i];
		} else if ( ! strcmp( argv[i], "-tg" ) ) {
			i++;
			testgauss = argv[i];
		} else if ( ! strcmp( argv[i], "-minx" ) ) {
			i++;
			sim.minx = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-maxx" ) ) {
			i++;
			sim.maxx = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-miny" ) ) {
			i++;
			sim.miny = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-maxy" ) ) {
			i++;
			sim.maxy = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-px" ) ) {
			i++;
			sim.px = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-py" ) ) {
			i++;
			sim.py = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-v" ) ) {
			i++;
			nvoters = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-n" ) ) {
			i++;
			sim.electionsPerPixel = strtol( argv[i], NULL, 10 );
		} else if ( ! strcmp( argv[i], "-Z" ) ) {
			i++;
			sim.voterSigma = strtod( argv[i], NULL );
		} else if ( ! strcmp( argv[i], "-c" ) ) {
			i++;
			sim.addCandidateArg( argv[i] );
		} else if ( ! strcmp( argv[i], "--list" ) ) {
			printEMList();
			exit(0);
		} else if ( ! strcmp( argv[i], "--manhattan" ) ) {
			sim.manhattanDistance = true;
		} else if ( ! strcmp( argv[i], "--linearFalloff" ) ) {
			sim.linearFalloff = true;
		} else if ( ! strcmp( argv[i], "--method" ) ) {
			const VSFactory* cf;
			i++;
			cf = VSFactory::byName( argv[i] );
			if ( cf == NULL ) {
				fprintf(stderr,"no such election method \"%s\", have:\n", argv[i] );
				printEMList();
				exit(1);
			}
			vs = cf->make();
		} else if ( ! strcmp( argv[i], "--opt" ) ) {
			i++;
			methodEnv[methodEnvCount] = argv[i];
			methodEnvCount++;
		} else if ( ! strcmp( argv[i], "--threads" ) ) {
			i++;
			nthreads = strtol( argv[i], NULL, 10 );
		} else {
			fprintf( stderr, "bogus arg \"%s\"\n", argv[i] );
			fputs( usage, stderr );
			fputs( "Known election methods:\n", stderr );
			printEMList();
			exit(1);
		}
	}

	if ( sim.candcount == 0 && testgauss == NULL ) {
		fprintf( stderr, "error, no candidate positions specified\n");
		exit(1);
	}
	if ( testgauss != NULL ) {
		sim.addCandidateArg("0,0");
		sim.build( 1 );
		sim.gaussTest( testgauss, nvoters );
		return 0;
	}
	if ( vs == NULL ) {
		vs = new InstantRunoffVotePickOne();
	}
	if ( methodEnvCount > 0 ) {
		vs->init( methodEnv );
	}
	sim.build( nvoters );
	{
	    char cfgstr[512];
	    sim.configStr( cfgstr, sizeof(cfgstr) );
	    fputs( cfgstr, stdout );
	    fputs( "\n", stdout );
	}
	if ( nthreads <= 1 ) {
		sim.run( vs );
	} else {
		PlaneSim* sims = new PlaneSim[nthreads-1];
		PlaneSimThread* threads = new PlaneSimThread[nthreads];
		int total = sim.px * sim.py;
		int step = total / nthreads;
		int pos = 0;
		int i;
		for (i = 0; i < nthreads-1; ++i) {
			sims[i].coBuild(sim);
			threads[i].sim = &(sims[i]);
			threads[i].vs = vs;
			threads[i].first = pos;
			pos += step;
			threads[i].last = pos - 1;
		}
		threads[i].sim = &sim;
		threads[i].vs = vs;
		threads[i].first = pos;
		threads[i].last = total - 1;
		for (i = 0; i < nthreads; ++i) {
			pthread_create(&(threads[i].thread), NULL, runPlaneSimThread, &(threads[i]));
		}
		for (i = 0; i < nthreads; ++i) {
			pthread_join(threads[i].thread, NULL);
		}
	}
	sim.writePNG( foutname );
}
