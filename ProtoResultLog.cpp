#include "ProtoResultLog.h"
#include "trial.pb.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

/* static */
ProtoResultLog* ProtoResultLog::open(const char* filename, int mode) {
	int fd = ::open(filename, mode, 0666);
	if (fd < 0) {
		perror(filename);
		return NULL;
	}
	return new ProtoResultLog(strdup(filename), fd);
}

static char* namefilename(const char* fname) {
	char* namefname = (char*)malloc(strlen(fname) + 7);
	strcpy(namefname, fname);
	strcat(namefname, ".names");
	return namefname;
}

// protected
ProtoResultLog::ProtoResultLog(char* fname_, int fd_) : fname(fname_), fd(fd_) {
	int err = pthread_mutex_init(&lock, NULL);
	assert(err == 0);
	{
		char* namefname = namefilename(fname);
		struct stat s;
		err = stat(namefname, &s);
		if (err >= 0) {
			int namefd = ::open(namefname, O_RDONLY);
			if (namefd >= 0) {
				NameBlock* nb = new NameBlock();
				nb->block = (char*)malloc(s.st_size);
				err = read(namefd, nb->block, s.st_size);
				assert(err == s.st_size);
				parseNameBlock(nb);
				names = nb;
				close(namefd);
			}
		}
		free(namefname);
	}
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

bool ProtoResultLog::useNames(NameBlock* nb) {
	if (names == NULL) {
		bool ok = true;
		char* namefname = namefilename(fname);
		names = nb;
		makeBlock(nb);
		int fdout = ::open(namefname, O_WRONLY|O_CREAT, 0444);
		if (fdout < 0) {
			perror(namefname);
			ok = false;
		} else {
			int err = write(fdout, nb->block, nb->blockLen);
			if (err != nb->blockLen) {
				perror("write");
				ok = false;
			}
			close(fdout);
		}
		free(namefname);
		return ok;
	} else {
		// check that passed in and loaded names are the same
		if (names->nnames != nb->nnames) {
			return false;
		}
		for (int i = 0; i < nb->nnames; ++i) {
			if (strcmp(names->names[i], nb->names[i])) {
				return false;
			}
		}
		return true;
	}
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

