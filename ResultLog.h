#ifndef RESULT_LOG_H
#define RESULT_LOG_H

#include "VoterSim.h"
#include "ResultFile.h"

class ResultLog {
public:
	
	class Result {
	public:
		int voters;
		int choices;
		double error;
		int seats;
		int systemIndex;
		VoterSim::PreferenceMode mode;
		int dimensions;
		double happiness;
		double voterHappinessStd;
		double gini;
		inline void clear() {
			voters = 0;
			choices = 0;
			error = -1.0;
			seats = 1;
			systemIndex = -1;
			mode = VoterSim::BOGUS_PREFERENCE_MODE;
			dimensions = -1;
			happiness = 0.0;
			voterHappinessStd = 0.0;
			gini = 0.0;
		}
		Result() {
			clear();
		}
	};
	ResultLog();
	virtual ~ResultLog();

	// do this once at setup.
	// returns true if everything is ok.
	// does not take ownership of nb.
	virtual bool useNames(NameBlock* nb);

	// do this for every trial election.
	// implementations should be thread safe.
#if 0
	virtual bool logResult(
		int voters, int choices, double error, int seats,
		int systemIndex, VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini) = 0;
#else
	virtual bool logResult(const Result& r) = 0;
#endif
	
	// return true if a result was read. false implies error or eof.
	// Not thread safe.
#if 0
	virtual bool readResult(
		int* voters, int* choices, double* error, int* seats,
		int* systemIndex, VoterSim::PreferenceMode* mode, int* dimensions,
		double* happiness, double* voterHappinessStd, double* gini) = 0;
#else
	virtual bool readResult(Result* r) = 0;
#endif
	
	// Return a copy of names.
	// caller responsible for delete of it.
	NameBlock* getNamesCopy();

protected:
	// System names
	NameBlock* names;
	
	// Defaults to false.
	bool delete_names;
	
public:
	inline const char* name(int i) {
		if (names == NULL) {
			return NULL;
		}
		return names->names[i];
	}
};

#endif
