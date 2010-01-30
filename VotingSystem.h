#ifndef VOTING_SYSTEM_H
#define VOTING_SYSTEM_H

class Voter;
class VoterArray;

class VotingSystem {
public:
    VotingSystem():name((char*)0){};
    VotingSystem(const char* nin):name(nin){};
    /*! @function runElection
	 * return winner in winnerArray[0], second place in [1], and so on */
    virtual void runElection( int* winnerArray, const VoterArray& they ) const = 0;
	/*! @var name
		Descriptive user-visible name. Often modulated by options set. */
    const char* name;
    
    /*! @function init
	 * null terminated array of char*, default action is to ignore all.
     * can take arbitrary options to vary election method.
	 * Compact the array and pass it to the superclass after using elements relevant to subclass.
     */
    virtual void init( const char** envp );

    // Like runElection but for multi-seat methods.
	// Default implementation calls runElection() if seats==1, returns false otherwise.
    // Return true unless there was an internal error.
    virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	
    /*! @function pickOneHappiness
	 * calculate average happiness of the voters for the winner */
    static double pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP, double* giniP, int start );
    static double pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP, int start );
    static double pickOneHappiness( const VoterArray& they, int numv, int winner, double* stddevP );
    static double pickOneHappiness( const VoterArray& they, int numv, int winner );

    virtual ~VotingSystem() = 0;
};

double multiseatHappiness( const VoterArray& they, int numv, int* winners, int seats, double* stddevP, double* giniP, int start = 0 );
// simpler version does much less work
double multiseatHappiness( const VoterArray& they, int numv, int* winners, int seats );

class VSFactory {
public:
    VotingSystem* (*fact)( const char* n );
    const char* name;
	
    inline VotingSystem* make() const {
		return fact( name );
    }
	
    VSFactory* next;
    static VSFactory* root;
	
    VSFactory( VotingSystem* (*f)( const char* ), const char* n );
	
    static const VSFactory* byName( const char* n );
	
	VSFactory* reverseList();
};

class VSConfig {
public:
    const VSFactory* fact;
    const char** args;
    VSConfig* next;
	
    VotingSystem* me;
	
    inline VSConfig( const VSFactory* f, const char** a, VSConfig* n = (VSConfig*)0 )
		: fact( f ), args( a ), next( n ), me( (VotingSystem*)0 )
    {}
    inline VSConfig( VotingSystem* vs, VSConfig* n = (VSConfig*)0 )
		: fact( (VSFactory*)0 ), args( (const char**)0 ), next( n ), me( vs )
    {}
    /*! @function newVSConfig
		* a list builting factory. root = newVSConfig( f, a, n );
     */
    static VSConfig* newVSConfig( const VSFactory* f, const char** a, VSConfig* n = (VSConfig*)0 );
    /*! @function newVSConfig
		* a list builting factory. root = newVSConfig( f, a, n );
     */
    inline static VSConfig* newVSConfig( const char* factoryName, const char** a, VSConfig* n = (VSConfig*)0 ) {
		return newVSConfig( VSFactory::byName( factoryName ), a, n );
    }
	
	inline VotingSystem* getVS() {
		if ( me == (VotingSystem*)0 ) {
			init();
		}
		return me;
	}
    /*! @function defaultList
		* return one of each in the factor list
     */
    static VSConfig* defaultList( const char** a, VSConfig* n );
    inline static VSConfig* defaultList() {
		return VSConfig::defaultList( (const char**)0, (VSConfig*)0 );
    }
	
    ~VSConfig();
	
    void print( void* f);
	
    void init();
	
    inline void runElection( int* winnerR, const VoterArray& they ) {
		me->runElection( winnerR, they );
    }
	
    inline const char* name() {
		return me->name;
    }
	
	VSConfig* reverseList( VSConfig* v = (VSConfig*)0 );
};

VSConfig* systemsFromDescFile( const char* filename, const char** methodArgs, int maxargc, VSConfig* systemList );

#endif
