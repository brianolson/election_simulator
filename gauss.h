#ifndef GAUSS_H
#define GAUSS_H

#ifdef __cplusplus
extern "C" {
#endif

double random_gaussian();
struct random_gaussian_context {
	int mode;
	double y2;
};
double random_gaussian_r(struct random_gaussian_context* context);

/* It's probably useful to do this:
struct random_gaussian_context gc = INITIAL_GAUSSIAN_CONTEXT;
double x = random_gaussian_r(&gc);
*/
#define INITIAL_GAUSSIAN_CONTEXT {0, 0.0}

#ifdef __cplusplus
}
#endif

#endif /* GAUSS_H */
