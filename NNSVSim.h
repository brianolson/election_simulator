#ifndef NNSVSIM_H
#define NNSVSIM_H

#include "NNStrategicVoter.h"
#include "DBResultFile.h"
#include <stdio.h>

#define strategies 0
#define voterDumpPrefix 0
#define voterBinDumpPrefix 0


//class Result;
class VotingSystem;

class NNSVSim {
public:
	void run( Result* r );
	void testg( int trialsPerGeneration = 50 );
	void nextg( double keep = 0.1 );
	void initPop( int numvIn, int numcIn );
	void print( FILE* f );
	
    double** happiness;	// double[nsys][trials]
    double* happisum;	// double[nsys]
    double* happistdsum;// double[nsys]
	int numv;
	int numc;
	int trials;
	
	VotingSystem** systems;
	int nsys;
	bool printVoters;
	bool printAllResults;
	FILE* text;
	bool doError;
	double confusionError;
#ifndef voterDumpPrefix
	char* voterDumpPrefix;
	int voterDumpPrefixLen;
#endif
#ifndef voterBinDumpPrefix
	char* voterBinDumpPrefix;
	int voterBinDumpPrefixLen;
#endif
	FILE* resultDump;
	FILE* resultDumpHtml;
	NNSVArray they;
	NNSVArray theyWithError;
	bool tweValid;
	printStyle summary;
	int resultDumpHtmlVertical;
//	int* winners;
	unsigned int upToTrials;
#ifndef strategies
	Strategy** strategies;
	int numStrat;
#endif
};

#endif
