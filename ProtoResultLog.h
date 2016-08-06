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
                        class GzipInputStream;
                        class GzipOutputStream;
		}
	}
}
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::GzipOutputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::GzipInputStream;
using google::protobuf::io::FileInputStream;

class ProtoResultLog : public ResultLog {
public:
	static ProtoResultLog* openForAppend(const char* filename);
	static ProtoResultLog* openForReading(const char* filename);
	virtual ~ProtoResultLog();
	virtual bool useNames(NameBlock* nb);
	virtual bool logResult(const TrialResult& r);
	virtual bool readResult(TrialResult* r);

protected:
	ProtoResultLog(char* fname_, int fd_);
	
	char* fname;
	int fd;
	pthread_mutex_t lock;
	FileInputStream* zcis;
	GzipInputStream* gzcis;
	CodedInputStream* cis;
	FileOutputStream* zcos;
	GzipOutputStream* gzcos;
	CodedOutputStream* cos;
	
	void setupForAppend();
	void setupForRead();
};

#endif
