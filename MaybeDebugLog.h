#ifndef MAYBE_DEBUG_LOG_H
#define MAYBE_DEBUG_LOG_H
#include <stdarg.h>

class MaybeDebugLog {
public:
	inline int vlog(const char* format, ...) {
		va_list ap;
		va_start(ap, format);
		int out = vvlog(format, ap);
		va_end(ap);
		return out;
	}
	virtual int vvlog(const char* format, va_list ap) = 0;
	virtual void clear() = 0;
	virtual void setEnable(bool enable) = 0;
	virtual bool isEnabled() = 0;
	virtual const char* name() = 0;
	virtual ~MaybeDebugLog();
	
	static MaybeDebugLog* ForPath(const char* path);
};

#endif /* MAYBE_DEBUG_LOG_H */
