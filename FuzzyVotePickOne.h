#ifndef FUZZYVOTEPICKONE_H
#define FUZZYVOTEPICKONE_H

#include "VotingSystem.h"

class FuzzyVotePickOne : public VotingSystem {
public:
    /** 0 none, 1 equal sum vote, 2 maximized */
    int doCounterweight;
    /** 0 none, -1 numc, N otherwise */
    int quantization;
    FuzzyVotePickOne( const char* name, int doCounterweightI, int quantizeI );
    virtual void init( const char** envp );
    virtual void runElection( int* winnerR, const VoterArray& they ) const;
};

#endif
