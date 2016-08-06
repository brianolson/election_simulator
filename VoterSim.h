#ifndef VOTER_SIM_H
#define VOTER_SIM_H

#include <stdio.h>
#include "Voter.h"
#include "ResultFile.h"

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

    void run( Result* );
    void runNoPrintcrap( Result* r );
    void run( ResultFile* drf, NameBlock& nb );
    void runFromWorkQueue( ResultFile* drf, NameBlock& nb, WorkSource* q );
	void oneTrial();

	// assign new random preferences to voters according to preferenceMode
	void randomizeVoters();

	// start = offset into they
	// count = number of they to process from they[start]
	double calculateHappiness(int start, int count, int* winners, double* stddevP, double* giniP);
	inline double calculateHappiness(
			int* winners, double* stddevP, double* giniP) {
		return calculateHappiness(0, numv, winners, stddevP, giniP);
	}

    double** happiness;	// double[nsys][trials]
    double* happisum;	// double[nsys]
    double* ginisum;	// double[nsys]
    double* happistdsum;// double[nsys]
    int numv;
    int numc;
    int trials;
	int seats;
    
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
	
	int currentTrialNumber;
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
	
	static inline const char* modeName(PreferenceMode m) {
		switch (m) {
			case BOGUS_PREFERENCE_MODE: return "Bogus Mode";
			case INDEPENDENT_PREFERENCES: return "Independent Prefs";
			case NSPACE_PREFERENCES: return "Flat Distribution Spatial";
			case NSPACE_GAUSSIAN_PREFERENCES: return "Gaussian Distribution Spatial";
			default: return "ERROR invalid preference value";
		}
	}

	void trialStrategySetup();
	void trailErrorSetup();
	bool validSetup();
	void preTrialOutput();
	void postTrialOutput();

	void preRunOutput();
	void postRunOutput();
	void doResultOut(Result* resultOut);
};

#endif
