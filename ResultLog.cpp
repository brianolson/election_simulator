#include "ResultLog.h"

ResultLog::ResultLog() : names(NULL), delete_names(false) {
}
ResultLog::~ResultLog() {
	if (delete_names && (names != NULL)) {
		delete names;
	}
}

bool ResultLog::useNames(NameBlock* nb) {
	names = nb;
	return true;
}

NameBlock* ResultLog::getNamesCopy() {
	if (names == NULL) {
		return NULL;
	}
	return names->clone();
}
