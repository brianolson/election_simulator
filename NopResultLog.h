#ifndef NOP_RESULT_LOG_H
#define NOP_RESULT_LOG_H

#include "ResultLog.h"

class NopResultLog : public ResultLog {
	public:
	NopResultLog();
	virtual ~NopResultLog();
	virtual bool logResult(const TrialResult& r);
	virtual bool readResult(TrialResult* r);
};

#endif /* NOP_RESULT_LOG_H */
