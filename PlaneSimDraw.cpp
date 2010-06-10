#include "PlaneSimDraw.h"

#include "GaussianRandom.h"
#include "PlaneSim.h"
#include "ResultAccumulation.h"

#include <assert.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>

PlaneSimDraw::PlaneSimDraw(int sizex, int sizey, int bytesPerPixel_)
: px(sizex), py(sizey), bytesPerPixel(bytesPerPixel_),
variableAccum(false) {
	pix = (uint8_t*)malloc(sizex * sizey * bytesPerPixel);
	assert(pix != NULL);
}
PlaneSimDraw::~PlaneSimDraw() {
	free(pix);
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


static void drawDiamond( int cx, int cy, int px, int py, const uint8_t* color, uint8_t* (*getpxp)( void* context, int x, int y ), void* context ) {
    int half = diamondSize / 2;
    int by = cy - half;
    int bx = cx - half;
    //printf("diamond at %d,%d #%02x%02x%02x\n", cx, cy, color[0], color[1], color[2] );
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
		p = getpxp( context, bx+x, by+y );
		*p = 0; p++;
		*p = 0; p++;
		*p = 0;
	    }	break;
	    case 2: {
		uint8_t* p;
		p = getpxp( context, bx+x, by+y );
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

static uint8_t* PC_getpxp( void* context, int x, int y ) {
	return static_cast<PlaneSimDraw*>(context)->getpxp(x, y);
}
void PlaneSimDraw::drawDiamond( int cx, int cy, const uint8_t* color ) {
	::drawDiamond(cx, cy, px, py, color, PC_getpxp, this);
}

static void writeImageDataToPNGFile( const char* outname, unsigned char** rows, int height, int width, const char* args );

// Write blended single winner pixels.
void PlaneSimDraw::writePNG( const char* filename, int numc, const ResultAccumulation* accum, int* candidateXY, const char* args ) {
	//assert(px == sim->px);
	//assert(py == sim->py);
	//const VoterArray& they = sim->they;
	//int numc = sim->they.numc;
	//const pos* candidates = sim->candidates;
	memset(pix, 0, px*py*bytesPerPixel);
	uint8_t** rows = new uint8_t*[py];
	//int i;
	//int sumErrors = 0;
	//double targetSum = sim->electionsPerPixel * sim->seats;
	fprintf(stderr,"making image %dx%d for %d choices\n", px, py, numc );
	for ( int i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double sum;
			sum = 0.0;
			for ( int i = 0; i < numc; i++ ) {
				sum += accum->getAccum( x, y, i );
			}
#if 0
			if ( (!variableAccum) && (sum != targetSum) ) {
				fprintf(stderr,"PlaneSimDraw::writePNG(%s) error at (%d,%d), sum %f != targetSum %f\n",
						filename, x, y, sum, targetSum );
				sumErrors++;
			}
#endif
			for ( int i = 0; i < numc; i++ ) {
				double weight = accum->getAccum( x, y, i ) / sum;
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
	if (candidateXY != NULL) {
		for ( int i = 0; i < numc; i++ ) {
#if 1
			drawDiamond(candidateXY[i*2], candidateXY[i*2 + 1], candidateColors + ((i % numCandidateColors) * 3));
#else
			printf("cd %f,%f -> ", candidates[i].x, candidates[i].y );
			drawDiamond( sim->xCoordToIndex( candidates[i].x ),
						sim->yCoordToIndex( candidates[i].y ),
						candidateColors + ((i % numCandidateColors) * 3) );
#endif
		}
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	rows = NULL;
	//delete [] pix;
	//pix = NULL;
#if 0
	if (sumErrors != 0) {
		fprintf(stderr, "%d targetSum errors out of (%dx%d=%d) pixels\n", sumErrors, px, py, px*py);
	}
#endif
}

// Write one plane of where a choice wins in multi-winner elections.
void PlaneSimDraw::writePlanePNG(
		const char* filename, int c, const ResultAccumulation* accum, int cpx, int cpy, const char* args ) {
	//assert(px == sim->px);
	//assert(py == sim->py);
	memset(pix, 0, px*py*bytesPerPixel);
	//const pos* candidates = sim->candidates;
	uint8_t** rows = new uint8_t*[py];
	int i;
	fprintf(stderr,"making image %dx%d for choices %d\n", px, py, c );
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double weight = accum->getAccum( x, y, c );
#if 1
			double sum = 0.0;
			for ( int i = 0; i < accum->getPlanes(); i++ ) {
				sum += accum->getAccum( x, y, i );
			}
			if (sum == 0.0) {
				weight = 0.0;
			} else {
				weight = weight / sum;
			}
#else
			weight = weight / sim->electionsPerPixel;
#endif
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
#if 1
	drawDiamond( cpx, cpy, candidateColors + ((c % numCandidateColors) * 3) );
#else
	printf("cd %f,%f -> ", candidates[c].x, candidates[c].y );
	drawDiamond( sim->xCoordToIndex( candidates[c].x ),
				sim->yCoordToIndex( candidates[c].y ),
				candidateColors + ((c % numCandidateColors) * 3) );
	char* args = new char[1024];
	if ( args != NULL ) {
	    sim->configStr( args, 1024 );
	}
#endif
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
}

// Sum across multiple winner planes.
void PlaneSimDraw::writeSumPNG( const char* filename, int numc, const ResultAccumulation* accum, int* candidateXY, const char* args, double targetSum ) {
	//assert(px == sim->px);
	//assert(py == sim->py);
	memset(pix, 0, px*py*bytesPerPixel);
	//const VoterArray& they = sim->they;
	//const pos* candidates = sim->candidates;
	uint8_t** rows = new uint8_t*[py];
	int i;
	//double targetSum = sim->electionsPerPixel * sim->seats;
	fprintf(stderr,"making sum image %dx%d\n", px, py );
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	for ( int y = 0; y < py; y++ ) {
		for ( int x = 0; x < px; x++ ) {
			double sum;
			sum = 0.0;
			for ( i = 0; i < numc; i++ ) {
				sum += accum->getAccum( x, y, i );
			}
			if ( sum > targetSum ) {
				fprintf(stderr,"error at (%d,%d), sum %f > targetSum %f\n",
						x, y, sum, targetSum );
			}
			for ( i = 0; i < numc; i++ ) {
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
	for ( i = 0; i < numc; i++ ) {
#if 1
		drawDiamond(candidateXY[i*2], candidateXY[i*2 + 1], candidateColors + ((i % numCandidateColors) * 3));
#else
	    printf("cd %f,%f -> ", candidates[i].x, candidates[i].y );
	    drawDiamond( sim->xCoordToIndex( candidates[i].x ),
					sim->yCoordToIndex( candidates[i].y ),
					candidateColors + ((i % numCandidateColors) * 3) );
#endif
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
}

void PlaneSimDraw::gaussTest( const char* filename, int nvoters, PlaneSim* sim, const char* args ) {
	assert(px == sim->px);
	assert(py == sim->py);
	int i;
	double dy, dx;
	int x, y;
	printf("generating %d random positions...\n", nvoters );
	ResultAccumulation accum(px, py, 1);
	accum.clear();
	for ( i = 0; i < nvoters; i++ ) {
		dy = sim->gRandom->get() * 0.5;
		dx = sim->gRandom->get() * 0.5;
		y = sim->yCoordToIndex( dy );
		x = sim->xCoordToIndex( dx );
		if ( y >= 0 && y < py && x >= 0 && x < px ) {
			accum.incAccum( x, y, 0 );
		}
	}
	printf("stored %d random positions\n", i );
	//pix = new uint8_t[px*py*3];
	uint8_t** rows = new uint8_t*[py];
	for ( i = 0; i < py; i++ ) {
		rows[i] = pix + 3*px*i;
	}
	int max = 0;
	int myi = -1, mxi = -1;
	for ( y = 0; y < py; y++ ) {
		for ( x = 0; x < px; x++ ) {
			int t = accum.getAccum( x, y, 0 );
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
			value = 255 - (uint8_t)((255.0 * accum.getAccum(x,y,0)) / max);
			uint8_t* p;
			p = getpxp( x, y );
			*p += value;
			p++;
			*p += value;
			p++;
			*p += value;
		}
	}
	writeImageDataToPNGFile( filename, rows, px, py, args );
	delete [] rows;
	//delete [] pix;
	//pix = NULL;
}


png_voidp user_error_ptr = 0;
static void user_error_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "error: %s", str );
}
static void user_warning_fn( png_structp png_ptr, const char* str ) {
	fprintf( stderr, "warning: %s", str );
}

static void writeImageDataToPNGFile( const char* outname, unsigned char** rows, int height, int width, const char* args ) {
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
		t.text = strdup(args);
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
