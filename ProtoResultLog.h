#ifndef PROTO_RESULT_LOG_H
#define PROTO_RESULT_LOG_H

#include "ResultLog.h"

#include <pthread.h>

class ProtoResultLog : public ResultLog {
public:
	static ProtoResultLog* open(const char* filename, int mode);
	virtual ~ProtoResultLog();
	virtual bool useNames(NameBlock* nb);
	virtual void logResult(
		int voters, int choices, double error, int systemIndex, 
		VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini);
	
protected:
	ProtoResultLog(char* fname_, int fd_);
	
	char* fname;
	int fd;
	pthread_mutex_t lock;
};

#endif
