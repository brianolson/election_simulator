#ifndef VOTER_H
#define VOTER_H

#include <stdio.h>

class Strategy;

class Voter {
protected:
    // one preference slot per candidate
    float* preference;
    int preflen;

public:
    
    Voter();
    Voter( int numCandidates );
    ~Voter();
    
//    static int defaultNumCandidates;
    
    // f is actually FILE*
    // pretty print text
    void print( void* f ) const;
    // parse friendly text
    void dump( void* f ) const;
    void undump( void* fi, int len );
    // binary
    void write( void* f ) const;
    void read( void* f, int len );

    inline float getPref( int i ) const {
        return preference[i];
    }
    inline int numc() const {
	return preflen;
    }
	inline void setPref( int i, float v ) {
		preference[i] = v;
	}
    
    // return index of highest preference
    int getMax() const;
    // return index of lowest preference
    int getMin() const;
    
    /* For each candidate preference, randomly add/subtract as much as error,
       then bound to -1.0 to 1.0 */
    void setWithError( const Voter& it, float error );
    
    void randomize();
    void randomizeWithFirstChoice( int first );

    void setStorage( float* p, int pl );
    void clearStorage();

    void set( const Voter& it, Strategy* s );
};

class Result;

class Strategy {
public:
    int count;
    Strategy* next;
    double** happiness;	// double[nsys][trials]
    double* happisum;	// double[nsys]
    double* ginisum;	// double[nsys]
    double* happistdsum;// double[nsys]
    Result* r;

    virtual void apply( float* dest, const Voter& src );
    virtual const char* name();
    Strategy();
    virtual ~Strategy();
};

class VoterArray {
public:
    VoterArray();
    VoterArray( int nv, int nc );
    void build( int nv, int nc );
    ~VoterArray();
    void clear();

    int numv;
    int numc;
protected:
    Voter* they;
    float* storage;

public:
    inline Voter& operator[]( int i ) const {
	return *(they + i);
    };
};

void voterDump( char* voteDumpFilename, const VoterArray& they, int numv, int numc );
void voterDump( FILE* voteDumpFile, const VoterArray& they, int numv, int numc );
void voterBinDump( char* voteDumpFilename, const VoterArray& they, int numv, int numc );

#endif
