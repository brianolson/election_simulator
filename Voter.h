#ifndef VOTER_H
#define VOTER_H

#include <stdio.h>
#include <stdlib.h>

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

	// random position within -1..1 N-cube
	static void randomUniformCoord(float* coord, int dimensions);
	// random gaussian position centered at 0
	static void randomGaussianCoord(float* coord, int dimensions, double sigma);
	
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
	
	// preferences for each voter are numc independent uniform random variables from -1..1
	void randomize();

	// place the voters in N-dimensional space, uniformly distributed within a -1..1 N-cube.
	// choicePositions is interpreted as 2D, [choice one x, y, z, ...][choice two x, y, z, ...]...
	// center can be NULL or double[dimensions] defining the center of the N-cube.
	// scale multiplies each position to change the size of the N-cube.
	// prefenece for a choice is (approvalDistance - voter_choice_distance)
	void randomizeNSpace(int dimensions, double* choicePositions, double* center = NULL, double scale = 1.0, double approvalDistance = 0.5);

	// place the voters in N-dimensional space, uniformly distributed within a gaussian distribution.
	// choicePositions is interpreted as 2D, [choice one x, y, z, ...][choice two x, y, z, ...]...
	// center can be NULL or double[dimensions] defining the center of the N-cube.
	// prefenece for a choice is (sigma - voter_choice_distance)
	void randomizeGaussianNSpace(int dimensions, double* choicePositions, double* center = NULL, double sigma = 1.0);

	// poisitions is written into, must be allocated double[numc*dimensions]
	static void randomGaussianChoicePositions(double* positions, int numc, int dimensions, double sigma = 1.0);
};

void voterDump( char* voteDumpFilename, const VoterArray& they, int numv, int numc );
void voterDump( FILE* voteDumpFile, const VoterArray& they, int numv, int numc );
void voterBinDump( char* voteDumpFilename, const VoterArray& they, int numv, int numc );

// [-1.0 .. 1.0]
inline double uniformOneOneRandom() {
#if 1
	// random() returns [0..2147483647]
	return
	(random() * (2.0/2147483647.0)) // [0..2.0]
	- 1.0;
#else
	// drand48 only available on some systems? deprecated?
	return (drand48() - 0.5) * 2.0;
#endif
}


#endif
