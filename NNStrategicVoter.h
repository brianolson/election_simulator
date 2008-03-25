#ifndef NN_STRATEGIC_VOTER_H
#define NN_STRATEGIC_VOTER_H

#include "Voter.h"

/*!
@class NNStrategicVoter
*/
class NNStrategicVoter : public Voter {
protected:
	/*! @var ranking
	* ranking[numc]
	* so we don't have to sort every time */
	//unsigned char* ranking;
	/*! @var nn
	* nn[numc*2]
	* not a fully connected NN, the only links are straight across as a gain factor on preference per rank,
	* and an offset per rank.
	* Possibly extend to numc*3 to provide a perceptron from 'poll results'.
	* NN inputs: ( ( 1st pref, 2nd pref, ... ) ( 1st poll rating, 2nd poll rating, ... ) bias-1 )
	* NN output: ( 1st pref, 2nd pref, ... ) */
	float* nn;
	/*! @var hidden
	how many hidden nodes are there between inputs and outputs (whose lengths are known by numc) */
	int hidden;
	/*! @var realPref */
	float* realPref;
	
public:
	/*! @var strategySuccess
	fitness, the sum of happiness over some trial set. */
	float strategySuccess;

	NNStrategicVoter();
	NNStrategicVoter( int numcIn );
	static int floatCount( int numc, int hidden );
    void setStorage( float* p, int pl );
    void clearStorage();
	static int nnLen( int numc, int hidden );
    void randomize();
    void randomizeNN();
	void setHonest();
	inline float getPref( int i ) const {
		return preference[i];
	}
	inline float getRealPref( int i ) const {
        return realPref[i];
    }
	void calcStrategicPref( double* pollRating );
	void setRealPref( float* realPrefIn );
	void set( const NNStrategicVoter& it );
	/*! @method mutate
		as long as a uniform random number in 0..1 is under rate, keep doing point mutations.
		@param rate 0..1, higher => more mutation, try around .2
		*/
	void mutate( double rate );
	/*! @method mixWith
		crossover mutation which assigns to this, which should probably be a copy of another NNSV */
	void mixWith( const NNStrategicVoter& it );

    // f is actually FILE*
    // pretty print text
    void print( void* f ) const;
    void printRealPref( void* f ) const;
    void printNN( void* f ) const;
    void printStrategicPref( void* f ) const;
    // parse friendly text
    void dump( void* f ) const;
    void undump( void* fi, int len );
    // binary
    void write( void* f ) const;
    void read( void* f, int len );
};


class NNSVArray : public VoterArray {
public:
    NNSVArray();
    NNSVArray( int nv, int nc );
    void build( int nv, int nc );
    ~NNSVArray();
    void clear();
	
    int numv;
    int numc;
protected:
	NNStrategicVoter* they;
    float* storage;
	
public:
	inline NNStrategicVoter& operator[]( int i ) const {
		return *(they + i);
	};
};
#endif
