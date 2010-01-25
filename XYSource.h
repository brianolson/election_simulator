#ifndef XY_SOURCE_H
#define XY_SOURCE_H

#include <assert.h>
#include <pthread.h>

class XYSource {
public:
	XYSource(int x, int y) : curx(0), cury(0), sizex(x), sizey(y) {
#if !NDEBUG
		int err = 
#endif
		pthread_mutex_init(&lock, NULL);
		assert(0 == err);
	}
	~XYSource() {
		pthread_mutex_destroy(&lock);
	}
	
	bool next(int* x, int* y);

	/**
	 * @param xy pointer to int array, store x,y pairs in it
	 * @param count xy is int[count*2], store up to this many pairs
	 * @return number of x,y pairs set int *xy. 0 on end.
	 */
	int nextN(int* xy, int count);

private:
	int curx;
	int cury;
	int sizex;
	int sizey;
	pthread_mutex_t lock;
};

#endif
