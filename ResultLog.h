#ifndef RESULT_LOG_H
#define RESULT_LOG_H

#include "VoterSim.h"
#include "ResultFile.h"

class ResultLog {
public:
	ResultLog();
	virtual ~ResultLog();

	// do this once at setup.
	// returns true if everything is ok.
	virtual bool useNames(NameBlock* nb);

	// do this for every trial election.
	// implementations should be thread safe.
	virtual void logResult(
		int voters, int choices, double error, int systemIndex, 
		VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini) = 0;

protected:
	// System names
	NameBlock* names;
};

#endif
