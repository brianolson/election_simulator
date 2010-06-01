#ifndef RESULT_ACCUMULATION_H
#define RESULT_ACCUMULATION_H 

class ResultAccumulation {
public:
	ResultAccumulation(int x, int y, int z)
	: accum((int*)0), px(x), py(y), planes(z) {
		accum = new int[px * py * planes];
	}
	~ResultAccumulation() {
		delete [] accum;
	}
	inline int getAccum( int x, int y, int c ) const {
		return accum[c + x*planes + y*planes*px];
	}
	inline void incAccum( int x, int y, int c ) {
		accum[c + x*planes + y*planes*px]++;
	}
	inline void setAccum( int x, int y, int c, int v ) {
		accum[c + x*planes + y*planes*px] = v;
	}
	
	void clear();
	
	inline int getPx() const {
		return px;
	}
	inline int getPy() const {
		return py;
	}
	inline int getPlanes() const {
		return planes;
	}
	
protected:
	int* accum;
	int px;
	int py;
	int planes;
};

#endif /* RESULT_ACCUMULATION_H */