#include "ResultFile.h"
#include "VotingSystem.h"
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

void votingSystemArrayToNameBlock( NameBlock* ret, VotingSystem** systems, int nsys ) {
    int len = 2;
    int i;
    char* block;
    for ( i = 0; i < nsys; i++ ) {
	len += strlen( systems[i]->name ) + 1;
    }
    block = (char*)malloc(len);
    ret->block = block;
    ret->blockLen = len;
    ret->names = (char**)malloc(sizeof(char*) * (nsys + 1) );
    if ( ret->names == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return;
    }
    ret->nnames = nsys;
    for ( i = 0; i < nsys; i++ ) {
	const char* name;
	ret->names[i] = block;
	name = systems[i]->name;
	while ( *name ) {
	    *block = *name;
	    name++;
	    block++;
	}
	*block = *name;
	block++;
    }
    *block = '\0';
    ret->names[i] = NULL;
}

void makeBlock( NameBlock* names ) {
    int i;
	
    names->blockLen = 1;
    for ( i = 0; i < names->nnames; i++ ) {
		names->blockLen += strlen( names->names[i] ) + 1;
    }
    names->block = (char*)malloc( names->blockLen );
    char* cur = names->block;
    for ( i = 0; i < names->nnames; i++ ) {
		char* src;
		src = names->names[i];
		*cur = *src;
		while ( *src ) {
			cur++;
			src++;
			*cur = *src;
		}
		cur++;
    }
    *cur = '\0';
}

void parseNameBlock( NameBlock* it ) {
    int i;
    int nnames = 0;
    for ( i = 0; i < it->blockLen; i++ ) {
	if ( it->block[i] == '\0' ) {
	    nnames++;
	    if ( (i + 1 < it->blockLen) && (it->block[i+1] == '\0') ) {
		break;
	    }
	}
    }
    it->nnames = nnames;
    it->names = (char**)malloc( sizeof(char*) * (nnames + 1));
    if ( it->names == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return;
    }
    it->names[0] = it->block;
    nnames = 0;
    for ( i = 0; i < it->blockLen; i++ ) {
	if ( it->block[i] == '\0' ) {
	    nnames++;
	    if ( (i + 1 < it->blockLen) && (it->block[i+1] == '\0') ) {
		break;
	    }
	    it->names[nnames] = it->block + i + 1;
	}
    }
    it->names[nnames] = NULL;
}

Result* newResult( int nsys ) {
    Result* toret = (Result*)malloc( 8 + nsys * sizeof(struct SystemResult) );
    if ( toret == NULL ) {
	fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	return NULL;
    }
    toret->trials = 0;
    for ( int i = 0; i < nsys; i++ ) {
	toret->systems[i].meanHappiness = 0;
	toret->systems[i].reliabilityStd = 0;
	toret->systems[i].consensusStdAvg = 0;
	toret->systems[i].giniWelfare = 0;
    }
    return toret;
}

void mergeResult( Result* a, Result* b, int nsys ) {
    int j;
    u_int32_t tsum = a->trials + b->trials;
    for ( j = 0; j < nsys; j++ ) {
	a->systems[j].meanHappiness = (a->systems[j].meanHappiness * a->trials + b->systems[j].meanHappiness * b->trials) / tsum;
	a->systems[j].reliabilityStd = (a->systems[j].reliabilityStd * a->trials + b->systems[j].reliabilityStd * b->trials) / tsum;
	a->systems[j].consensusStdAvg = (a->systems[j].consensusStdAvg * a->trials + b->systems[j].consensusStdAvg * b->trials) / tsum;
    }
    a->trials = tsum;
}

void printSystemResult( void* file, const struct SystemResult* it, const char* name, printStyle style ) {
    FILE* f = (FILE*)file;
    if ( style == smallBasic ) {
	// slightly smaller summary
	fprintf( f, "%s\t%.9g\t%.9g\t%.9g\t%.9g\n", name, it->meanHappiness, it->reliabilityStd, it->consensusStdAvg, it->giniWelfare );
    } else if ( style == smallerBasic ) {
	// slightly further smaller summary
	fprintf( f, "%p\t%.9g\t%.9g\t%.9g\t%.9g\n", it, it->meanHappiness, it->reliabilityStd, it->consensusStdAvg, it->giniWelfare );
    } else if ( style == html || style == htmlWithStrategy ) {
	// html table summary
	fprintf( f, "<tr><td>%s</td><td>%.9g</td><td>%.9g</td><td>%.9g</td><td>%.9g</td></tr>\n", name, it->meanHappiness, it->reliabilityStd, it->consensusStdAvg, it->giniWelfare );
    } else if ( style != noPrint ) {
	// standard summary
	fprintf( f, "%s\tmean happiness\t%.9g\tstd\t%.9g\tavg std\t%.9g\tGini Index\t%.9g\n", name, it->meanHappiness, it->reliabilityStd, it->consensusStdAvg, it->giniWelfare );
    }
}

void printResult( void* file, const Result* r, char** names, int nsys, printStyle style ) {
    int i;
    for ( i = 0; i < nsys; i++ ) {
	printSystemResult( file, r->systems + i, names ? names[i] : NULL, style );
    }
}

void printResultHeader( void* file, u_int32_t numc, u_int32_t numv, u_int32_t trials, float error, int nsys, printStyle style ) {
    FILE* f = (FILE*)file;
    if ( style == html ) {
        fprintf( f, "%d candidates, %d voters, %d trial elections, +/- %f confusion error, %d systems<br><table><tr><th>Name</th><th>Avg. Happiness<br>(higher better)</th><th>Reliability Std.<br>(lower better)</th><th>Consensus Std.<br>(lower better)</th><th>Gini Index<br>(lower better)</th></tr>\n", numc, numv, trials, error, nsys );
    } else if ( style == htmlWithStrategy ) {
        fprintf( f, "%d candidates, %d voters, %d trial elections, +/- %f confusion error, %d systems<br><table><tr><th>Name</th><th>Strategy</th><th>Avg. Happiness<br>(higher better)</th><th>Reliability Std.<br>(lower better)</th><th>Consensus Std.<br>(lower better)</th><th>Gini Index<br>(lower better)</th></tr>\n", numc, numv, trials, error, nsys );
    } else if ( style != noPrint ) {
	if ( error < 0 ) {
	    fprintf( f, "%d candidates, %d voters, %d trial elections, %d systems\n",
	      numc, numv, trials, nsys );
	} else {
	    fprintf( f, "%d candidates, %d voters, %d trial elections, +/- %f confusion error, %d systems\n",
	      numc, numv, trials, error, nsys );
	}
    }
}
void printResultFooter( void* file, u_int32_t numc, u_int32_t numv, u_int32_t trials, float error, int nsys, printStyle style ) {
    FILE* f = (FILE*)file;
    if ( style == html || style == htmlWithStrategy ) {
        fprintf( f, "<tr><th>Name</th><th>Avg. Happiness<br>(higher better)</th><th>Reliability Std.<br>(lower better)</th><th>Consensus Std.<br>(lower better)</th><th>Gini Index<br>(lower better)</th></tr></table>\n" );
    }
}

ResultFile::~ResultFile() {
}

TextDumpResultFile::TextDumpResultFile() : f(NULL), names(NULL) {
}
TextDumpResultFile* TextDumpResultFile::open(const char* filename) {
	FILE* f = fopen(filename, "a");
	if (f == NULL) {
		perror(filename);
		return NULL;
	}
	TextDumpResultFile* out = new TextDumpResultFile();
	out->f = f;
	return out;
}
TextDumpResultFile::~TextDumpResultFile() {
	close();
}

/**
 * This implementation always returns NULL. put-only.
 */
Result* TextDumpResultFile::get( int choices, int voters, float error ) {
	return NULL;
}

// Does not take ownership of it, copies if needed.
int TextDumpResultFile::put(Result* it, int choices, int voters, float error) {
	assert(names);
	printResultHeader(f, choices, voters, it->trials, error, names->nnames, smallBasic);
	printResult(f, it, (names == NULL) ? NULL : names->names, names->nnames, smallBasic);
	fflush((FILE*)f);
	return 0;
}

int TextDumpResultFile::close() {
	int err = 0;
	if (f != NULL) {
		err = fclose((FILE*)f);
		f = NULL;
		if (err < 0) {
			perror("TextDumpResultFile::~TextDumpResultFile");
		}
	}
	return err;
}
int TextDumpResultFile::flush() {
	fflush((FILE*)f);
	return 0;
}

int TextDumpResultFile::useNames( const NameBlock* namesIn ) {
	FILE* fout = (FILE*)f;
	names = namesIn;
	fprintf(fout, "num-systems: %d\n", names->nnames);
	for (int i = 0; i < names->nnames; ++i) {
		fprintf(fout, "system-name: %s\n", names->names[i]);
	}
	return 0;
}

int TextDumpResultFile::useStrategyNames( const NameBlock* namesIn ) {
	fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
	FILE* fout = (FILE*)f;
	fprintf(fout, "num-strategies: %d\n", namesIn->nnames);
	for (int i = 0; i < namesIn->nnames; ++i) {
		fprintf(fout, "strategy-name: %s\n", namesIn->names[i]);
	}
	return 0;
}
