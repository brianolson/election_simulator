#include "ProtoResultLog.h"
#include "trial.pb.h"

#include <stdio.h>
#include <fcntl.h>

/* static */
ProtoResultLog* ProtoResultLog::open(const char* filename, int mode) {
	int fd = ::open(filename, mode, 0666);
	if (fd < 0) {
		perror(filename);
		return NULL;
	}
	return new ProtoResultLog(strdup(filename), fd);
}
ProtoResultLog::~ProtoResultLog() {
	if (fd >= 0) {
		close(fd);
	}
	if (fname) {
		free(fname);
	}
	pthread_mutex_destroy(&lock);
}

inline TrialResult::Model trFromVS(VoterSim::PreferenceMode m) {
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

void ProtoResultLog::logResult(
		int voters, int choices, double error, int systemIndex, 
		VoterSim::PreferenceMode mode, int dimensions,
		double happiness, double voterHappinessStd, double gini) {
	TrialResult r;
	
	r.set_voters(voters);
	r.set_choices(choices);
	r.set_error(error);
	r.set_system_index(systemIndex);
	r.set_voter_model(trFromVS(mode));
	r.set_dimensions(dimensions);
	
	r.set_mean_happiness(happiness);
	r.set_voter_happiness_stddev(voterHappinessStd);
	r.set_gini_index(gini);

#if 0
	fprintf(stderr, "%d\t%d\t%f\t%d\t%d\t%d\t%f\t%f\t%f\n",
		voters, choices, error, systemIndex, mode, dimensions,
		happiness, voterHappinessStd, gini);
#endif
	pthread_mutex_lock(&lock);
	assert(r.SerializeToFileDescriptor(fd));
	pthread_mutex_unlock(&lock);
}

// protected
ProtoResultLog::ProtoResultLog(char* fname_, int fd_) : fname(fname_), fd(fd_) {
	int err = pthread_mutex_init(&lock, NULL);
	assert(err == 0);
}
