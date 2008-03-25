#include "gauss.h"
#include <stdlib.h>
#include <math.h>

#ifdef USE_OLD_RAND
/* only use this if your libc doesn't have the newer better random() */
static __inline double ranf() {
	return rand() / (RAND_MAX * 1.0);
}
#else
static __inline double ranf() {
	return random() / 2147483648.0;
}
#endif
double random_gaussian() {
    static int mode = 0;
    static double y2;
    double x1, x2, w/*, y1*/;//, y2;

    if ( mode == 1 ) {
	mode = 0;
	return y2;
    }
	do {
		x1 = 2.0 * ranf() - 1.0;
		x2 = 2.0 * ranf() - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );
	
	w = sqrt( (-2.0 * log( w ) ) / w );
	y2 = x2 * w;
	mode = 1;
	/*y1 =*/ return x1 * w;
}
