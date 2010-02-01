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
	void gaussTest( const char* filename, int nvoters, PlaneSim* sim );
	void writePNG( const char* filename, PlaneSim* sim, const ResultAccumulation* accum );
	void writePlanePNG( const char* filename, int choice, PlaneSim* sim, const ResultAccumulation* accum );
	void writeSumPNG( const char* filename, PlaneSim* sim, const ResultAccumulation* accum );
	
	void drawDiamond( int x, int y, const uint8_t* color );
};

#endif /* PLANE_SIM_DRAW_H */
