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
