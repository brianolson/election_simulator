#include "NameBlock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

NameBlock::NameBlock()
: block(NULL), blockLen(0),
names(NULL), nnames(0) {
}

NameBlock::~NameBlock() {
	if (block != NULL) {
		free(block);
	}
	if (names != NULL) {
		free(names);
	}
}

bool NameBlock::parse(char* data, unsigned int len) {
	blockLen = len;
	block = data;
	int i;
	if (names != NULL) {
		free(names);
	}
	nnames = 0;
    for ( i = 0; i < blockLen; i++ ) {
		if ( block[i] == '\0' ) {
			nnames++;
			if ( (i + 1 < blockLen) && (block[i+1] == '\0') ) {
				break;
			}
		}
    }
    names = (char**)malloc( sizeof(char*) * (nnames + 1));
    if ( names == NULL ) {
		fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
		free(data);
		block = NULL;
		return false;
    }
    names[0] = block;
    nnames = 0;
    for ( i = 0; i < blockLen; i++ ) {
		if ( block[i] == '\0' ) {
			nnames++;
			if ( (i + 1 < blockLen) && (block[i+1] == '\0') ) {
				break;
			}
			names[nnames] = block + i + 1;
		}
    }
    names[nnames] = NULL;
	return true;
}

bool NameBlock::setNames(char** nameList, int numNames) {
	names = nameList;
	nnames = numNames;
	
	if (block != NULL) {
		free(block);
	}
    int i;
	
    blockLen = 1;
    for ( i = 0; i < nnames; i++ ) {
		blockLen += strlen( names[i] ) + 1;
    }
    block = (char*)malloc( blockLen );
	if (block == NULL) {
		return false;
	}
    char* cur = block;
    for ( i = 0; i < nnames; i++ ) {
		char* src;
		src = names[i];
		*cur = *src;
		while ( *src ) {
			cur++;
			src++;
			*cur = *src;
		}
		cur++;
    }
    *cur = '\0';
	return true;
}

bool NameBlock::copy(const NameBlock& a) {
	if (block != NULL) {
		free(block);
		block = NULL;
	}
	if (names != NULL) {
		free(names);
		names = NULL;
	}
	char* nb = (char*)malloc(a.blockLen);
	if (nb == NULL) {
		return false;
	}
	memcpy(nb, a.block, a.blockLen);
	return parse(nb, a.blockLen);
}
