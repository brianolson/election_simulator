#ifndef VOTER_SIM_H
#define VOTER_SIM_H

#include <stdio.h>
#include "Voter.h"
#include "ResultFile.h"

#ifndef NO_DB
class DBResultFile;
#endif
class Result;
class NameBlock;
class VotingSystem;
class WorkSource;
class Strategy;
class ResultFile;
class ResultLog;

class VoterSim {
public:
    int init( int argc, char** argv );
	
	// arg is the name of the long option, without the '--' prefix.
	// argc_after is the number of arguments after the long option name.
	// argv_after is the options after the long option.
	// return -1 on failure, number of argv elements to skip otherwise (0 or more).
	int setLongOpt(const char* arg, int argc_after, char** argv_after);
    
    VoterSim();
    ~VoterSim();

#ifndef NO_DB
    void run( DBResultFile* drf, NameBlock& nb );
    void runSteps( DBResultFile* drf, NameBlock& nb );
    void runFromWorkQueue( DBResultFile* drf, NameBlock& nb, WorkSource* q );
#endif
    void run( Result* );
    void runNoPrintcrap( Result* r );
    void run( ResultFile* drf, NameBlock& nb );
    void runFromWorkQueue( ResultFile* drf, NameBlock& nb, WorkSource* q );

	// assign new random preferences to voters according to preferenceMode
	void randomizeVoters();

    double** happiness;	// double[nsys][trials]
    double* happisum;	// double[nsys]
    double* ginisum;	// double[nsys]
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
    char* voterDumpPrefix;
    int voterDumpPrefixLen;
    char* voterBinDumpPrefix;
    int voterBinDumpPrefixLen;
    FILE* resultDump;
    FILE* resultDumpHtml;
    VoterArray they;
    VoterArray theyWithError;
    bool tweValid;
	printStyle summary;
    int resultDumpHtmlVertical;
    int* winners;
    unsigned int upToTrials;
    Strategy** strategies;
    int numStrat;
	
	ResultLog* rlog;
	
	enum PreferenceMode {
		BOGUS_PREFERENCE_MODE = 0,
		INDEPENDENT_PREFERENCES = 1,
		NSPACE_PREFERENCES = 2,
		NSPACE_GAUSSIAN_PREFERENCES = 3,
		PREFERENCE_MODE_LIMIT = 4,
	};
	PreferenceMode preferenceMode;
	int dimensions;
};

#endif
