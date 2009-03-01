#ifndef PROTO_RESULT_LOG_H
#define PROTO_RESULT_LOG_H

#include "ResultLog.h"

#include <pthread.h>

namespace google {
	namespace protobuf {
		namespace io {
			class FileInputStream;
			class FileOutputStream;
			class CodedOutputStream;
			class CodedInputStream;
		}
	}
}
class ProtoResultLog : public ResultLog {
public:
	static ProtoResultLog* openForAppend(const char* filename);
	static ProtoResultLog* openForReading(const char* filename);
	virtual ~ProtoResultLog();
	virtual bool useNames(NameBlock* nb);
	virtual bool logResult(
		int voters, int choices, double error, int systemIndex, 
		VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini);
	
	// return true if a result was read. false implies error or eof.
	virtual bool readResult(
		int* voters, int* choices, double* error, int* systemIndex,
		VoterSim::PreferenceMode* mode, int* dimensions,
		double* happiness, double* voterHappinessStd, double* gini);
protected:
	ProtoResultLog(char* fname_, int fd_);
	
	char* fname;
	int fd;
	pthread_mutex_t lock;
	google::protobuf::io::FileInputStream* zcis;
	google::protobuf::io::CodedInputStream* cis;
	google::protobuf::io::FileOutputStream* zcos;
	google::protobuf::io::CodedOutputStream* cos;
	
	void setupForAppend();
	void setupForRead();
};

#endif
