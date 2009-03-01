#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include <map>
#include <set>
#include <vector>

#include "trial.pb.h"
#include "ProtoResultLog.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>

using google::protobuf::int32;
using google::protobuf::io::FileInputStream;

using std::vector;
using std::set;
using std::map;

class ResultList {
public:
	vector<float> mean_happiness;
	vector<float> voter_happiness_stddev;
	vector<float> gini_index;
	
	// average total happiness
	double mean_avg;
	// average voter variability
	double voter_std_avg;
	// average voter variability
	double gini_avg;
	// system reliability
	double mean_std;
	
	void process() {
		int i;
		double mean_sum = 0.0;
		double mean_vs = 0.0;
		double voter_std_sum = 0.0;
		double gini_sum = 0.0;
		for (i = 0; i < mean_happiness.size(); ++i) {
			mean_sum += mean_happiness[i];
		}
		mean_avg = mean_sum / mean_happiness.size();
		for (i = 0; i < voter_happiness_stddev.size(); ++i) {
			voter_std_sum += voter_happiness_stddev[i];
		}
		voter_std_avg = voter_std_sum / voter_happiness_stddev.size(); 
		for (i = 0; i < gini_index.size(); ++i) {
			gini_sum += gini_index[i];
		}
		gini_avg = gini_sum / voter_happiness_stddev.size(); 
		for (i = 0; i < mean_happiness.size(); ++i) {
			double v = mean_happiness[i] - mean_avg;
			mean_vs += v * v;
		}
		mean_std = sqrt(mean_vs / mean_happiness.size());
	}
};

class ResultHolder {
public:
	// Parameters
	int32 voters;
	int32 choices;
	float error;
	int32 system_index;
	VoterSim::PreferenceMode mode;
	int32 dimensions;
	
	// Results
	ResultList* results;
	ResultHolder()
	: voters(-1), choices(-1), error(-1.0), system_index(-1), mode(VoterSim::BOGUS_PREFERENCE_MODE), dimensions(-1), results(NULL) {
	}
	ResultHolder(int32 v, int32 c, float e, int32 i, VoterSim::PreferenceMode m, int32 d)
	: voters(v), choices(c), error(e), system_index(i), mode(m), dimensions(d), results(NULL) {
	}
	
	void addResult(double mean_happiness, double voter_happiness_stddev, double gini_index) {
		if (results == NULL) {
			results = new ResultList();
		}
		results->mean_happiness.push_back(mean_happiness);
		results->voter_happiness_stddev.push_back(voter_happiness_stddev);
		results->gini_index.push_back(gini_index);
	}
};

class ResultHolderCompare {
public:
	int operator()(const ResultHolder& a, const ResultHolder& b) {
		// is a less than b?
		if (a.voters < b.voters) {
			return 1;
		}
		if (a.voters > b.voters) {
			return 0;
		}
		if (a.choices < b.choices) {
			return 1;
		}
		if (a.choices > b.choices) {
			return 0;
		}
		if (a.error < b.error) {
			return 1;
		}
		if (a.error > b.error) {
			return 0;
		}
		if (a.system_index < b.system_index) {
			return 1;
		}
		if (a.system_index > b.system_index) {
			return 0;
		}
		if (a.mode < b.mode) {
			return 1;
		}
		if (a.mode > b.mode) {
			return 0;
		}
		if (a.dimensions < b.dimensions) {
			return 1;
		}
		if (a.dimensions > b.dimensions) {
			return 0;
		}
		return 0;
	}
};

typedef map<ResultHolder, ResultHolder, ResultHolderCompare> ResultsMap;
ResultsMap results;
set<int> vsteps;
set<int> csteps;
set<double> esteps;
set<int> sisteps;
set<VoterSim::PreferenceMode> modesteps;
set<int> dimsteps;

template<class T> void printIterable(T i, const T& end, const char* format) {
	for (; i != end; i++) {
		printf(format, *i);
	}
}

int main(int argc, char** argv) {
	int count = 0;
	int voters, choices, systemIndex, dimensions;
	double error, happiness, voterHappinessStd, gini;
	VoterSim::PreferenceMode mode;

	ProtoResultLog* rlog = ProtoResultLog::openForReading(argv[1]);
	while (rlog->readResult(&voters, &choices, &error, &systemIndex, &mode, &dimensions, &happiness, &voterHappinessStd, &gini)) {
		count++;
		ResultHolder key(voters, choices, error, systemIndex, mode, dimensions);
		ResultHolder& value = results[key];
		value.addResult(happiness, voterHappinessStd, gini);
	}
	printf("read %d records in %d configurations\n", count, results.size());
	count = 0;
	for (ResultsMap::iterator ri = results.begin(); ri != results.end(); ri++) {
		{
			const ResultHolder& rh = (*ri).first;
			//printf("v%d c%d e%f si%d m%d d%d\n", rh.voters, rh.choices, rh.error, rh.system_index, rh.mode, rh.dimensions);
			vsteps.insert(rh.voters);
			csteps.insert(rh.choices);
			esteps.insert(rh.error);
			sisteps.insert(rh.system_index);
			modesteps.insert(rh.mode);
			dimsteps.insert(rh.dimensions);
		}
		{
			ResultHolder& rh = (*ri).second;
			rh = (*ri).second;
			rh.results->process();
		}
		count++;
	}
	printf("iterated %d configurations\n", count);
	printf("%d vsteps:", vsteps.size());
	printIterable(vsteps.begin(), vsteps.end(), " %d");
	printf("\n%d csteps:", csteps.size());
	printIterable(csteps.begin(), csteps.end(), " %d");
	printf("\n%d esteps:", esteps.size());
	printIterable(esteps.begin(), esteps.end(), " %.2f");
	printf("\n%d sisteps:", sisteps.size());
	printIterable(sisteps.begin(), sisteps.end(), " %d");
	printf("\n%d modesteps:", modesteps.size());
	printIterable(modesteps.begin(), modesteps.end(), " %d");
	printf("\n%d dimsteps:", dimsteps.size());
	printIterable(dimsteps.begin(), dimsteps.end(), " %d");
	printf("\n");
}

