#include "DBResultFile.h"
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

ResultKeyStruct::ResultKeyStruct( int c, int v, float e )
    : choices( c ), voters( v ), error( e )
{}


/**
 * Magic Keys
 */
/* C strings */
static char* SNN = "SYSTEM_NAMES";
DBT SYSTEM_NAMES = { SNN, 12 };
/* float array */
static char* ESN = "ERROR_STEPS";
DBT ERROR_STEPS = { ESN, 11 };
/* int array */
static char* CSN = "CHOICE_STEPS";
DBT CHOICE_STEPS = { CSN, 12 };
/* int array */
static char* VSN = "VOTER_STEPS";
DBT VOTER_STEPS = { VSN, 11 };
/* C strings */
static char* STN = "STRATEGY_NAMES";
DBT STRATEGY_NAMES = { STN, 14 };
/* int */
static char* VEN = "RESULT_DB_VERSION";
DBT RESULT_DB_VERSION = { VEN, 17 };

//#define db ((DB*)theDB)

DBResultFile* DBResultFile::open( const char* filename ) {
    return open( filename, O_CREAT|O_RDWR );
}
DBResultFile* DBResultFile::open( const char* filename, int flags ) {
    DBResultFile* toret = NULL;
    DB* f;
    f = dbopen( filename, flags, 0777, DB_HASH, NULL );
    if ( f == NULL ) {
	perror("dbopen");
	return NULL;
    }
    toret = new DBResultFile( f );
    if ( toret->openInit() != 0 ) {
	fprintf(stderr,"\nopenInit fails\n");
	delete toret;
	return NULL;
    }
    return toret;
}

DBResultFile::DBResultFile( void* dbIn )
    : db( (DB*)dbIn )
{}
int DBResultFile::openInit() {
    int err;
    err = db_get( db, &SYSTEM_NAMES, &(systemNames), 0 );
    if ( err == 0 ) {
	char* block = (char*)malloc(systemNames.size);
	if ( block == NULL ) {
	    fprintf(stderr,"%s:%d malloc failed\n", __FILE__, __LINE__ );
	    return -1;
	}
	memcpy( block, (char*)systemNames.data, systemNames.size );;
	//names.blockLen = systemNames.size;
//	printf("got name block %p size %d\n", names.block, names.blockLen );
	//parseNameBlock( &names );
	names.parse(block, systemNames.size);
#if 0
	printf("opening db with system names:\n");
	for ( int i = 0; i < names.nnames; i++ ) {
	    printf("%s\n", names.names[i] );
	}
#endif
    } else if ( err == DB_NOTFOUND ) {
	systemNames.size = 0;
	systemNames.data = NULL;
	names.block = NULL;
	names.blockLen = 0;
	names.names = NULL;
	names.nnames = 0;
    } else {
	perror("DBResultFile getting system names");
	fprintf(stderr,"err=%d\n", err);
	return -1;
    }
    err = db_get( db, &ERROR_STEPS, &(errorSteps), 0 );
    if ( err == 0 ) {
    } else if ( err == DB_NOTFOUND ) {
	errorSteps.size = 0;
	errorSteps.data = NULL;
    } else {
	perror("DBResultFile getting ERROR_STEPS");
    }
    err = db_get( db, &CHOICE_STEPS, &(choiceSteps), 0 );
    if ( err == 0 ) {
    } else if ( err == DB_NOTFOUND ) {
	choiceSteps.size = 0;
	choiceSteps.data = NULL;
    } else {
	perror("DBResultFile getting CHOICE_STEPS");
    }
    err = db_get( db, &VOTER_STEPS, &(voterSteps), 0 );
    if ( err == 0 ) {
    } else if ( err == DB_NOTFOUND ) {
	voterSteps.size = 0;
	voterSteps.data = NULL;
    } else {
	perror("DBResultFile getting VOTER_STEPS");
    }
#if 0
    DBT rdbvt;
    err = db_get( db, &RESULT_DB_VERSION, &(rdbvt), 0 );
    if ( err == 0 && rdbvt.size == 4 ) {
	resultDBVersion = *((u_int32_t*)(rdbvt.data));
    } else if ( err == DB_NOTFOUND ) {
	voterSteps.size = 0;
	voterSteps.data = NULL;
    } else {
	fprintf(stderr,"err=%d rdbvt.size=%d\n", err, rdbvt.size );
	perror("DBResultFile getting RESULT_DB_VERSION");
    }
#endif
    return 0;
}

int DBResultFile::close() {
    int toret = 0;
    if ( db != NULL ) {
	toret = db->close( db, 0 );
	db = NULL;
    }
    return toret;
}
int DBResultFile::flush() {
    return db->sync( db, 0 );
}
DBResultFile::~DBResultFile() {
    close();
}

Result* DBResultFile::get( int choices, int voters, float error ) {
    ResultKeyStruct rks( choices, voters, error );
    DBT key = { &rks, sizeof(rks) };
    DBT d;
    int err;
    d.flags = DB_DBT_MALLOC;
    err = db_get( db, &key, &d, 0 );
    if ( err == 0 ) {
	assert( d.size == sizeofRusult(numSystems()) );
	return (Result*)d.data;
    }
    if ( err != DB_NOTFOUND ) {
	fprintf(stderr,"%s:%d drf->get err=%d\n", __FILE__, __LINE__, err );
    }
    return NULL;
}

int DBResultFile::put( Result* it, int choices, int voters, float error ) {
    ResultKeyStruct rks( choices, voters, error );
    DBT key = { &rks, sizeof(rks) };
    DBT d = { it, sizeofRusult(numSystems()) };
    int err;
    err = db_put( db, &key, &d, 0 );
    if ( err != 0 ) {
	db->err(db,err,"%s:%d put result ", __FILE__, __LINE__ );
	//perror("DBResultFile put");
	return err;
    }
    /* check that c, v and e are in the list of knowns */
    /* but, it's not crucial. could just read all the keys for such data. */
    return err;
}

int DBResultFile::useNames( const NameBlock* namesIn ) {
    if ( names.block == NULL ) {
#if 0
	printf("initting to names:\n");
	for( int i = 0; i < namesIn->nnames; i++ ) {
	    printf("%s\n", namesIn->names[i] );
	}
#endif
	names.clone(*namesIn);
	/*names.nnames = namesIn->nnames;
	names.names = namesIn->names;
	makeBlock( &names );*/
	systemNames.data = names.block;
	systemNames.size = names.blockLen;
//	printf("putting names %p size %d\n", systemNames.data, systemNames.size );
	int err = db_put( db, &SYSTEM_NAMES, &systemNames, 0 );
	if ( err == -1 ) { perror("DBResultFile putting systemNames"); }
	return err;
    }
    /* check that names are same as in db */
    for ( int i = 0; i < namesIn->nnames; i++ ) {
	bool match;
	match = false;
	for ( int j = 0; j < names.nnames; j++ ) {
	    if ( strcmp( names.names[j], namesIn->names[i] ) == 0 ) {
		match = true;
		break;
	    }
	}
	if ( !match ) {
	    fprintf( stderr, "DBResultFile doesn't match system names to be used\n"
		"new name \"%s\" has no match in DB\n", namesIn->names[i] );
	    return 1;
	}
    }
    return 0;
}

int DBResultFile::useStrategyNames( const NameBlock* namesIn ) {
    if ( strategyNames.block == NULL ) {
#if 0
	printf("initting to names:\n");
	for( int i = 0; i < namesIn->nnames; i++ ) {
	    printf("%s\n", namesIn->names[i] );
	}
#endif
	strategyNames.nnames = namesIn->nnames;
	strategyNames.names = namesIn->names;
	makeBlock( &strategyNames );
	strategyDBT.data = strategyNames.block;
	strategyDBT.size = strategyNames.blockLen;
	//	printf("putting strategyNames %p size %d\n", strategyDBT.data, strategyDBT.size );
	int err = db_put( db, &SYSTEM_NAMES, &strategyDBT, 0 );
	if ( err == -1 ) { perror("DBResultFile putting strategyDBT"); }
	return err;
    }
    /* check that strategyNames are same as in db */
    for ( int i = 0; i < namesIn->nnames; i++ ) {
	bool match;
	match = false;
	for ( int j = 0; j < strategyNames.nnames; j++ ) {
	    if ( strcmp( strategyNames.names[j], namesIn->names[i] ) == 0 ) {
		match = true;
		break;
	    }
	}
	if ( !match ) {
	    fprintf( stderr, "DBResultFile doesn't match strategy names to be used\n"
	      "new name \"%s\" has no match in DB\n", namesIn->names[i] );
	    return 1;
	}
    }
    return 0;
}
