#ifndef RESULT_LOG_H
#define RESULT_LOG_H

#include "VoterSim.h"
#include "ResultFile.h"
#include "trial.pb.h"

class ResultLog {
public:

	ResultLog();
	virtual ~ResultLog();

	// do this once at setup.
	// returns true if everything is ok.
	// does not take ownership of nb.
	virtual bool useNames(NameBlock* nb);

	// do this for every trial election.
	// implementations should be thread safe.
	virtual bool logResult(const TrialResult& r) = 0;
	
	// return true if a result was read. false implies error or eof.
	// Not thread safe.
	virtual bool readResult(TrialResult* r) = 0;
	
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

static inline TrialResult::Model trFromVS(VoterSim::PreferenceMode m) {
	switch (m) {
	case VoterSim::INDEPENDENT_PREFERENCES:
		return TrialResult::INDEPENDENT_PREFERENCES;
	case VoterSim::NSPACE_PREFERENCES:
		return TrialResult::NSPACE_PREFERENCES;
	case VoterSim::NSPACE_GAUSSIAN_PREFERENCES:
		return TrialResult::NSPACE_GAUSSIAN_PREFERENCES;
	default:
		assert(0);
	}
	return (TrialResult::Model)-1;
}

static inline VoterSim::PreferenceMode vsFromTR(TrialResult::Model m) {
	switch (m) {
		case TrialResult::INDEPENDENT_PREFERENCES:
			return VoterSim::INDEPENDENT_PREFERENCES;
		case TrialResult::NSPACE_PREFERENCES:
			return VoterSim::NSPACE_PREFERENCES;
		case TrialResult::NSPACE_GAUSSIAN_PREFERENCES:
			return VoterSim::NSPACE_GAUSSIAN_PREFERENCES;
		default:
			assert(0);
	}
	return VoterSim::BOGUS_PREFERENCE_MODE;
}

#endif
