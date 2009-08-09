#include "ProtoResultLog.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "trial.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

using google::protobuf::uint32;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;

/* static */
ProtoResultLog* ProtoResultLog::openForAppend(const char* filename) {
	int fd = ::open(filename, O_APPEND|O_WRONLY|O_CREAT, 0666);
	if (fd < 0) {
		perror(filename);
		return NULL;
	}
	ProtoResultLog* out = new ProtoResultLog(strdup(filename), fd);
	out->setupForAppend();
	return out;
}
ProtoResultLog* ProtoResultLog::openForReading(const char* filename) {
	int fd = -1;
	ProtoResultLog* out;
	if ((filename == NULL) || (!strcmp(filename, "-"))) {
		fd = STDIN_FILENO;
		filename = "";
	} else {
		fd = ::open(filename, O_RDONLY);
		if (fd < 0) {
			perror(filename);
			return NULL;
		}
	}
	out = new ProtoResultLog(strdup(filename), fd);
	out->setupForRead();
	return out;
}

void ProtoResultLog::setupForAppend() {
	zcos = new FileOutputStream(fd);
	cos = new CodedOutputStream(zcos);
}
void ProtoResultLog::setupForRead() {
	zcis = new FileInputStream(fd);
	cis = new CodedInputStream(zcis);
	cis->SetTotalBytesLimit(1024*1024*1024, 1024*1024*1024);
}

static char* namefilename(const char* fname) {
	char* namefname = (char*)malloc(strlen(fname) + 7);
	strcpy(namefname, fname);
	strcat(namefname, ".names");
	return namefname;
}

// protected
ProtoResultLog::ProtoResultLog(char* fname_, int fd_)
	: ResultLog(), fname(fname_), fd(fd_),
	zcis(NULL), cis(NULL), zcos(NULL), cos(NULL)
{
	int err = pthread_mutex_init(&lock, NULL);
	assert(err == 0);
	{
		// load names from file
		char* namefname = namefilename(fname);
		struct stat s;
		err = stat(namefname, &s);
		if (err >= 0) {
			fprintf(stderr, "loading names from \"%s\" ...", namefname);
			int namefd = ::open(namefname, O_RDONLY);
			if (namefd >= 0) {
				NameBlock* nb = new NameBlock();
				char* block = (char*)malloc(s.st_size);
				err = read(namefd, block, s.st_size);
				assert(err == s.st_size);
				//nb->blockLen = err;
				//parseNameBlock(nb);
				nb->parse(block, s.st_size);
				names = nb;
				delete_names = true;
				close(namefd);
				fprintf(stderr, "done.\n");
			} else {
				perror(namefname);
			}
		}
		free(namefname);
	}
}

ProtoResultLog::~ProtoResultLog() {
	if (cos != NULL) delete cos;
	if (zcos != NULL) delete zcos;
	if (cis != NULL) delete cis;
	if (zcis != NULL) delete zcis;
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
		// use these names, write out to file.
		bool ok = true;
		char* namefname = namefilename(fname);
		fprintf(stderr, "writing out names to \"%s\" ...", namefname);
		names = nb;
		//makeBlock(nb);
		int fdout = ::open(namefname, O_WRONLY|O_CREAT, 0644);
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
			fprintf(stderr, "done\n");
		}
		free(namefname);
		return ok;
	} else {
		fprintf(stderr, "comparing passed in names to existing\n");
		// check that passed in and loaded names are the same
		if (names->nnames != nb->nnames) {
			fprintf(stderr, "mismatch in names. loaded has %d and new run has %d\n", names->nnames, nb->nnames);
			return false;
		}
		for (int i = 0; i < nb->nnames; ++i) {
			if (strcmp(names->names[i], nb->names[i])) {
				fprintf(stderr, "mismatch at name %d: old \"%s\" != new \"%s\"\n", i, names->names[i], nb->names[i]);
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

inline VoterSim::PreferenceMode vsFromTR(TrialResult::Model m) {
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

static inline void myperror_(int en, const char* x, const char* file, int line) {
	char es[256];
	strerror_r(en, es, sizeof(es));
	fprintf(stderr, "%s:%d %s: %s\n", file, line, x, es);
}
#define myperror(a,b) myperror_(a, b, __FILE__, __LINE__)


bool ProtoResultLog::logResult(
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

	bool ok = true;
#if 0
	fprintf(stderr, "%d\t%d\t%f\t%d\t%d\t%d\t%f\t%f\t%f\n",
		voters, choices, error, systemIndex, mode, dimensions,
		happiness, voterHappinessStd, gini);
#endif
	pthread_mutex_lock(&lock);
	if (cos != NULL) {
		cos->WriteVarint32(r.ByteSize());
		ok = r.SerializeToCodedStream(cos);
		if (!ok) {
			myperror(zcos->GetErrno(), fname);
		}
	} else {
		fprintf(stderr, "output is not setup\n");
		ok = false;
	}
	pthread_mutex_unlock(&lock);
	return ok;
}

// return true if a result was read. false implies error or eof.
bool ProtoResultLog::readResult(
		int* voters, int* choices, double* error, int* systemIndex,
		VoterSim::PreferenceMode* mode, int* dimensions,
		double* happiness, double* voterHappinessStd, double* gini) {
	TrialResult r;
	if (cis == NULL) return false;
	uint32 size;
	bool ok = cis->ReadVarint32(&size);
	if (!ok) return ok;
	CodedInputStream::Limit l = cis->PushLimit(size);
	ok = r.ParseFromCodedStream(cis);
	cis->PopLimit(l);
	if (!ok) return ok;
	if (ok) {
		*voters = r.voters();
		*choices = r.choices();
		*error = r.error();
		*systemIndex = r.system_index();
		*mode = vsFromTR(r.voter_model());
		*dimensions = r.dimensions();
		*happiness = r.mean_happiness();
		*voterHappinessStd = r.voter_happiness_stddev();
		*gini = r.gini_index();
	}
	return ok;
}
