#include "NopResultLog.h"

NopResultLog::NopResultLog() {}

NopResultLog::~NopResultLog() {}

bool NopResultLog::logResult(const Result& r) {
	return true;
}
bool NopResultLog::readResult(Result* r) {
	return false;
}
