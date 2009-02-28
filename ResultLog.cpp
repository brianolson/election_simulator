#include "ResultLog.h"

ResultLog::ResultLog() : names(NULL) {
}
ResultLog::~ResultLog() {
}

bool ResultLog::useNames(NameBlock* nb) {
	names = nb;
}
