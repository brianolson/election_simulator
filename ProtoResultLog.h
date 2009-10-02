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
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;

class ProtoResultLog : public ResultLog {
public:
	static ProtoResultLog* openForAppend(const char* filename);
	static ProtoResultLog* openForReading(const char* filename);
	virtual ~ProtoResultLog();
	virtual bool useNames(NameBlock* nb);
#if 0
	virtual bool logResult(
		int voters, int choices, double error, int seats, int systemIndex, 
		VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini);
	
	// return true if a result was read. false implies error or eof.
	virtual bool readResult(
		int* voters, int* choices, double* error, int* seats, int* systemIndex,
		VoterSim::PreferenceMode* mode, int* dimensions,
		double* happiness, double* voterHappinessStd, double* gini);
#else
	virtual bool logResult(const Result& r);
	virtual bool readResult(Result* r);
#endif

protected:
	ProtoResultLog(char* fname_, int fd_);
	
	char* fname;
	int fd;
	pthread_mutex_t lock;
	FileInputStream* zcis;
	CodedInputStream* cis;
	FileOutputStream* zcos;
	CodedOutputStream* cos;
	
	void setupForAppend();
	void setupForRead();
};

#endif
