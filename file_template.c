#include <stdlib.h>
#include <string.h>

#include "file_template.h"

// return a newly malloc()ed string with path templating done.
// replace "%m" with methodName
// replace "%p" with decimal plane number
char* fileTemplate(const char* tpl, const char* methodName, const char* plane) {
	if (methodName == NULL) {
		methodName = "";
	}
	if (plane == NULL) {
		plane = "";
	}
	const char* methodIndex = strstr(tpl, "%m");
	const char* planeIndex = strstr(tpl, "%p");
	int newlen = strlen(tpl) + 1;
	if (methodIndex != NULL) {
		newlen += strlen(methodName) - 2;
	}
	if (planeIndex != NULL) {
		// allocate enough for any decimal int32
		newlen += strlen(plane) - 2;
	}
	char* out = (char*)malloc(newlen);
	const char* in = tpl;
	char* pos = out;
	while (*in != '\0') {
		if (in[0] == '%') {
			if (in[1] == 'm') {
				in += 2;
				while (*methodName != '\0') {
					*pos = *methodName;
					pos++;
					methodName++;
				}
			} else if (in[1] == 'p') {
				in += 2;
				while (*plane != '\0') {
					*pos = *plane;
					pos++;
					plane++;
				}
			} else {
				*pos = *in;
				in++;
				pos++;
			}
		} else {
			*pos = *in;
			in++;
			pos++;
		}
	}
	*pos = '\0';
	return out;
}
