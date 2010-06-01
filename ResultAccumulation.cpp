#include "ResultAccumulation.h"
#include <string.h>

void ResultAccumulation::clear() {
	memset(accum, 0, sizeof(int) * px * py * planes);
}
