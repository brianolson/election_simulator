//#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MaybeDebugLog.h"

// assert that is always on. NDEBUG doesn't kill it.
#define CHECK(n) if (!(n)){fprintf(stderr, "%s:%d \"%s\" failed\n", __FILE__, __LINE__, #n);exit(1);}

class FILEMaybeDebugLog : public MaybeDebugLog {
public:
	// Takes copy of input, making it safe to release after call.
	FILEMaybeDebugLog(const char* name)
	: out(NULL), enable(false), buffer(NULL), first(NULL), last(NULL) {
		CHECK(name != NULL);
		outname = strdup(name);
		CHECK(0 == pthread_mutex_init(&lock, NULL));
	}
	// Takes copy of name, making it safe to release after call.
	// 'name' is optional and may be NULL.
	FILEMaybeDebugLog(FILE* outfile, const char* name = NULL)
	: enable(false), buffer(NULL), first(NULL), last(NULL) {
		CHECK(outfile != NULL);
		if (name == NULL) {
			outname = NULL;
		} else {
			outname = strdup(name);
		}
		out = outfile;
		CHECK(0 == pthread_mutex_init(&lock, NULL));
	}
	virtual ~FILEMaybeDebugLog() {
		clear();
		takeLock();
		if (outname != NULL) {
			free(outname);
		}
		if (out != NULL) {
			fclose(out);
		}
		if (buffer != NULL) {
			free(buffer);
		}
		releaseLock();
		pthread_mutex_destroy(&lock);
	}

	virtual int vvlog(const char* format, va_list ap) {
		takeLock();
		if (enable) {
			if (out == NULL) {
				CHECK(outname);
				out = fopen(outname, "a");
				if (out == NULL) {
					perror(outname);
                                        releaseLock();
					return -1;
				}
			}
			int ret = vfprintf(out, format, ap);
			releaseLock();
			return ret;
		}
		if (buffer == NULL) {
			buflen = 64*1024;
			buffer = (char*)malloc(buflen);
			CHECK(buffer);
		}
		int len = vsnprintf(buffer, buflen, format, ap);
		if (last == NULL) {
			first = last = new Node(buffer, len, NULL);
		} else {
			last = new Node(buffer, len, last);
		}
		releaseLock();
		return len;
	}
	virtual void clear() {
		takeLock();
		Node* cur = first;
		Node* next;
		while (cur != NULL) {
			next = cur->next;
			delete cur;
			cur = next;
		}
		first = NULL;
		last = NULL;
		releaseLock();
	}
	virtual void setEnable(bool enable_) {
		takeLock();
		enable = enable_;
		if (enable) {
			if (out == NULL) {
				CHECK(outname);
				out = fopen(outname, "a");
				CHECK(out);
			}
			Node* cur = first;
			Node* next;
			while (cur) {
				fwrite(cur->data, 1, cur->len, out);
				next = cur->next;
				delete cur;
				cur = next;
			}
			first = last = NULL;
		}
		releaseLock();
	}
	virtual bool isEnabled() {
		return enable;
	}
	virtual const char* name() {
		return outname;
	}

private:
	char* outname;
	FILE* out;
	bool enable;
	char* buffer;
	int buflen;
	pthread_mutex_t lock;
	inline void takeLock() {
		CHECK(0 == pthread_mutex_lock(&lock));
	}
	inline void releaseLock() {
		CHECK(0 == pthread_mutex_unlock(&lock));
	}
	class Node {
	public:
		char* data;
		size_t len;
		Node* next;
		
		Node(const char* in, size_t inlen, Node* prev)
		: next(NULL) {
			data = strdup(in);
			CHECK(data);
			len = inlen;
			if (prev != NULL) {
				prev->next = this;
			}
		}
		~Node() {
			free(data);
		}
	};
	
	Node* first;
	Node* last;
};

MaybeDebugLog::~MaybeDebugLog() {
}

class LoggerListNode {
public:
	MaybeDebugLog* logger;
	char* path;
	LoggerListNode* next;
	
	LoggerListNode(const char* pathIn, LoggerListNode* nextIn)
	: next(nextIn) {
		path = strdup(pathIn);
		logger = new FILEMaybeDebugLog(path);
	}
};

static pthread_mutex_t maybe_loggers_lock;
static pthread_once_t maybe_global_once = PTHREAD_ONCE_INIT;
static LoggerListNode* globalLoggers = NULL;
void initGlobalMaybeLogging(void) {
	CHECK(0 == pthread_mutex_init(&maybe_loggers_lock, NULL));
}

MaybeDebugLog* MaybeDebugLog::ForPath(const char* path) {
	pthread_once(&maybe_global_once, initGlobalMaybeLogging);
	CHECK(0 == pthread_mutex_lock(&maybe_loggers_lock));
	LoggerListNode* cur = globalLoggers;
	MaybeDebugLog* out = NULL;
	while (cur != NULL) {
		if (0 == strcmp(cur->path, cur->logger->name())) {
			out = cur->logger;
		}
	}
	if (out == NULL) {
		globalLoggers = new LoggerListNode(path, globalLoggers);
		CHECK(globalLoggers != NULL);
		out = globalLoggers->logger;
		CHECK(out != NULL);
	}
	CHECK(0 == pthread_mutex_unlock(&maybe_loggers_lock));
	return out;
}
