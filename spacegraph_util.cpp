#include "VotingSystem.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void printEMList(void) {
	VSFactory* cf;
	cf = VSFactory::root;
	while ( cf != NULL ) {
		printf("%s\n", cf->name );
		cf = cf->next;
	}
}

// return array of VotingSystem*, length in length_OUT param
VotingSystem** getVotingSystemsFromMethodNames(const char* methodNames, int* length_OUT) {
	int numVotingSystems = 1;
	VotingSystem** systems = NULL;
	const VSFactory* cf;
	char* vsnameScratch = strdup(methodNames);
	char* vsstart = vsnameScratch;
	char* pos = vsnameScratch;
	while (*pos != '\0') {
		if (*pos == ',') {
			numVotingSystems++;
		}
		pos++;
	}
	pos = vsnameScratch;
	systems = new VotingSystem*[numVotingSystems];
	int si = 0;
	while (*pos != '\0') {
		if (*pos == ',') {
			*pos = '\0';
			cf = VSFactory::byName(vsstart);
			if ( cf == NULL ) {
				fprintf(stderr,"no such election method \"%s\", have:\n", vsstart );
				printEMList();
				exit(1);
			}
			systems[si] = cf->make();
			si++;
			vsstart = pos + 1;
		}
		pos++;
	}
	cf = VSFactory::byName(vsstart);
	if ( cf == NULL ) {
		fprintf(stderr,"no such election method \"%s\", have:\n", vsstart );
		printEMList();
		exit(1);
	}
	systems[si] = cf->make();
	si++;
	assert(si == numVotingSystems);
	free(vsnameScratch);
	*length_OUT = numVotingSystems;
	return systems;
}

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
