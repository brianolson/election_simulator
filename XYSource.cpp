#include "XYSource.h"

bool XYSource::next(int* x, int* y) {
	bool ok = true;
	pthread_mutex_lock(&lock);
	if (cury >= sizey) {
		ok = false;
	} else {
		*x = curx;
		*y = cury;
		curx++;
		if (curx >= sizex) {
			curx = 0;
			cury++;
		}
	}
	pthread_mutex_unlock(&lock);
	return ok;
}

int XYSource::nextN(int* xy, int count) {
	int out = 0;
	pthread_mutex_lock(&lock);
	while ((out < count) && (cury < sizey)) {
		xy[0] = curx;
		xy[1] = cury;
		xy += 2;
		out++;
		curx++;
		if (curx >= sizex) {
			curx = 0;
			cury++;
		}
	}
	pthread_mutex_unlock(&lock);
	return out;
}

