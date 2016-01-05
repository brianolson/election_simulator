#ifndef RESULT_FILE_H
#define RESULT_FILE_H

#include <sys/types.h>
#include <stdint.h>

// struct on disk
struct SystemResult {
    double meanHappiness; // average
    double reliabilityStd; // stddev of averages
    double consensusStdAvg; // avegage of stddevs
    double giniWelfare; // gini welfare function
};
typedef struct SystemResult SystemResult; 

// struct on disk
/*!
 @struct Result
*/
class Result {
    public:
    u_int32_t trials;

    /*! @var systems size is numSystems * numStrategies
     * accessed as systems[strategy * numSystems + system] */
    SystemResult systems[1];
};
//typedef struct Result Result;


#if 0
// Result 3
//
// Result for a single trial of some parameters.
// A trial is one set of voters tested against various election methods and strategy-sets against an election method.
struct Result3 {
    double happinessAvg;
    double happinessStd;
    double happinessGini;
};

struct Params {
    uint32_t voters;
    uint32_t choices;
    uint32_t seats;
    double error;
};

struct Result3Set {
    // systems and results are arrays of length [numSystems]
    // C++ ish this would be std::vector<uint32_t> systems; std::vector<Result2> results;
    uint32_t numSystems;
    uint32_t* systems;
    Result3* results;
};

// Full record of one parameter trial and result
struct ParamResult {
    Params param;
    Result3Set result;
};
#endif

// File-per-param-values should store a list of Result2Set objects

#ifndef INLINE
#ifdef __cplusplus
#define INLINE inline
#else
#define INLINE __inline
#endif
#endif
INLINE size_t sizeofRusult( unsigned int nsys ) {
    return (sizeof(struct SystemResult) * nsys + sizeof(u_int32_t) * 2);
}
INLINE size_t sizeofRusult( unsigned int nsys, unsigned int nstrat ) {
    return (sizeof(struct SystemResult) * (nsys * (nstrat + 1)) + sizeof(u_int32_t) * 2);
}

typedef enum printStyle {
	noPrint = 0,
	basic = 1,
	smallBasic = 2,
	smallerBasic = 3,
	html = 4,
	htmlWithStrategy = 5,
	penultimatePrintStyle
} printStyle;

Result* newResult( int nsys );
// a = weighted average of a and b
void mergeResult( Result* a, Result* b, int nsys );
void printSystemResult( void* file, const struct SystemResult* it, const char* name, printStyle style = basic );
void printResult( void* file, const Result* it, char** names, int nsys, printStyle style = basic );
void printResultHeader( void* file, u_int32_t numc, u_int32_t numv, u_int32_t trials, float error, int seats, int nsys, printStyle style );
void printResultFooter( void* file, u_int32_t numc, u_int32_t numv, u_int32_t trials, float error, int seats, int nsys, printStyle style );

#include "NameBlock.h"

class VotingSystem;

// FIXME deprecated and delete these, replaced with NameBlock members.
void votingSystemArrayToNameBlock( NameBlock* ret, VotingSystem** systems, int nsys );
//void parseNameBlock( NameBlock* it );
//void makeBlock( NameBlock* names );

class ResultFile {
 public:
    virtual ~ResultFile();

    /**
     * malloc()s. caller's responsibility.
     */
    virtual Result* get( int choices, int voters, float error, int seats ) = 0;

  // Does not take ownership of it, copies if needed.
    virtual int put( Result* it, int choices, int voters, float error, int seats ) = 0;

    virtual int close() = 0;
    virtual int flush() = 0;

    virtual int useNames( const NameBlock* namesIn ) = 0;

    virtual int useStrategyNames( const NameBlock* namesIn ) = 0;
};

// A write-only Resultfile that just appends to a text file.
class TextDumpResultFile : public ResultFile {
 protected:
    TextDumpResultFile();

  // secretly FILE*
    void* f;
	const NameBlock* names;
 public:
    static TextDumpResultFile* open(const char* filename);
    virtual ~TextDumpResultFile();

    /**
     * This implementation always returns NULL. put-only.
     */
    virtual Result* get( int choices, int voters, float error, int seats );

  // Does not take ownership of it, copies if needed.
    virtual int put( Result* it, int choices, int voters, float error, int seats );

    virtual int close();
    virtual int flush();

    virtual int useNames( const NameBlock* namesIn );

    virtual int useStrategyNames( const NameBlock* namesIn );
};

#endif
