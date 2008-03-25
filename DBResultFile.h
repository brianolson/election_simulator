#ifndef DB_RESULT_FILE_H
#define DB_RESULT_FILE_H

#include "ResultFile.h"
#ifdef linux
//#include <db1/db.h>
#include <db.h>
#else
#include <db.h>
#endif

#include <sys/types.h>

class DBResultFile {
public:
    static DBResultFile* open( const char* filename );
    static DBResultFile* open( const char* filename, int flags );
    virtual ~DBResultFile();
    virtual int close();
    virtual int flush();

    /**
     * malloc()s. caller's responsibility.
     */
    virtual Result* get( int choices, int voters, float error );

    virtual int put( Result* it, int choices, int voters, float error );

    virtual int useNames( const NameBlock* namesIn );

    virtual int useStrategyNames( const NameBlock* namesIn );

    DBT systemNames;
    DBT errorSteps;
    DBT choiceSteps;
    DBT voterSteps;
    DBT strategyDBT;
    NameBlock names;
    NameBlock strategyNames;
    //int resultDBVersion;
    inline int numSystems() { return names.nnames; }
    inline int numStrategies() { return strategyNames.nnames; }
/*protected:*/
    DBResultFile( void* dbIn );
    int openInit();
    DB* db;
};

class ResultKeyStruct {
public:
    int32_t choices;
    int32_t voters;
    float error;
    ResultKeyStruct( int32_t c, int32_t v, float e );
    inline ResultKeyStruct(){};
};


#if defined(DB_VERSION_MAJOR) && DB_VERSION_MAJOR >= 4
#define DB4 1
inline DB* dbopen( const char* filename, int flags, int mode, DBTYPE type, void* a ) {
    DB* toret;
    int err;

    err = db_create( &toret, NULL, 0 );
    if ( err < 0 ) {
	fprintf(stderr,"\n%s:%d db_create fails with %d\n", __FILE__, __LINE__, err );
	return NULL;
    }
    err = toret->open( toret, NULL, filename, NULL, type, DB_CREATE, mode );
    if ( err < 0 ) {
	toret->errx(toret,"\nname=\"%s\"\n", filename);
	return NULL;
    }
    return toret;
}

#define db_get( db, key, data, flags ) ((db)->get( (db), NULL, (key), (data), (flags) ))
#define db_put( db, key, data, flags ) ((db)->put( (db), NULL, (key), (data), (flags) ))
#else
#define db_get( db, key, data, flags ) ((db)->get( (db), (key), (data), (flags) ))
#define db_put( db, key, data, flags ) ((db)->put( (db), (key), (data), (flags) ))
#endif

#endif
