#include "NopResultLog.h"

NopResultLog::NopResultLog() {}

NopResultLog::~NopResultLog() {}

bool NopResultLog::logResult(const TrialResult& r) {
	return true;
}
bool NopResultLog::readResult(TrialResult* r) {
	return false;
}
