#ifndef PLANE_SIM_DRAW_H
#define PLANE_SIM_DRAW_H

#include <assert.h>
#include <stdint.h>

class PlaneSim;
class ResultAccumulation;

class PlaneSimDraw {
public:
	int px;
	int py;
	int bytesPerPixel;
	uint8_t* pix;
	
	bool variableAccum;
	
	PlaneSimDraw(int sizex, int sizey, int bytesPerPixel_);
	~PlaneSimDraw();
	
	inline uint8_t* getpxp( int x, int y ) {
		assert(x >= 0);
		assert(x < px);
		assert(y >= 0);
		assert(y < py);
		return pix + (x*3 + y*bytesPerPixel*px);
	}
	inline void setpx( int x, int y, const uint8_t* color ) {
		int index = px*bytesPerPixel*y + bytesPerPixel*x;
		uint8_t* p = pix + index;
		int i = 0;
		while (true) {
			*p = *color;
			i++;
			if (i >= bytesPerPixel) {
				break;
			}
			p++;
			color++;
		}
	}
	void gaussTest( const char* filename, int nvoters, PlaneSim* sim, const char* args );
	void writePNG( const char* filename, int numc, /*PlaneSim* sim, */const ResultAccumulation* accum, int* candidateXY, const char* args );
	void writePlanePNG( const char* filename, int c, const ResultAccumulation* accum, int cpx, int cpy, const char* args );
	void writeSumPNG( const char* filename, PlaneSim* sim, const ResultAccumulation* accum, const char* args );
	
	void drawDiamond( int x, int y, const uint8_t* color );
};

#endif /* PLANE_SIM_DRAW_H */
