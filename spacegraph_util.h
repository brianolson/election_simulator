#ifndef SPACEGRAPH_UTIL_H
#define SPACEGRAPH_UTIL_H

void printEMList(void);
VotingSystem** getVotingSystemsFromMethodNames(const char* methodNames, int* length_OUT);
int readConfigFile(const char** configOutP, PlaneSim& sim, int* nvotersP, int* numVotingSystemsP, VotingSystem*** systemsP);
void testGauss(const char* path, PlaneSim* sim, int nvoters);

static const int nvoters_default = 1000;

#endif /* SPACEGRAPH_UTIL_H */
