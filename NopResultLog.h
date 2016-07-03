#ifndef NOP_RESULT_LOG_H
#define NOP_RESULT_LOG_H

#include "ResultLog.h"

class NopResultLog : public ResultLog {
	public:
	NopResultLog();
	virtual ~NopResultLog();
	virtual bool logResult(const Result& r);
	virtual bool readResult(Result* r);
};

#endif /* NOP_RESULT_LOG_H */
