#include "ResultFile.h"
#include "DBResultFile.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

inline int min( int a, int b ) {
    if ( a < b ) return a;
    return b;
}

#define BYTES_PER_LINE 16
#define BYTES_PER_CLUSTER 2

static int buffeq( char* a, char* b ) {
    int i;
    for ( i = 0; i < BYTES_PER_LINE; i++ )
        if ( a[i] != b[i] )
            return 0;
    return 1;
}

int chunk( char* buf, void** b, size_t* s ) {
    int len = min( BYTES_PER_LINE, *s );
    if ( len == 0 ) return len;
    memcpy( buf, *b, len );
    *((caddr_t*)(b)) += len;
    *s -= len;
    return len;
}

void hexprint( void* bufin, size_t sizin ) {
//    char c;
    int i,bc=0;
    int count = 0;
    char bufA[BYTES_PER_LINE],bufB[BYTES_PER_LINE];
    char *buf = bufA, *obuf;
    bc = chunk( buf, &bufin, &sizin );//read( 0, buf, BYTES_PER_LINE );
    while ( bc > 0 ) {
        printf("%x\t",count);
        for ( i = 0; ( i < BYTES_PER_LINE ) && ( i < bc ); i++ )
            printf("%.2x ",(unsigned char)buf[i]);
        for ( i = 0; ( i < BYTES_PER_LINE ) && ( i < bc ); i++ )
            if ( isprint( buf[i] ) )
                printf("%c",buf[i]);
            else
                printf(".");
        printf("\n");
        if ( buf == bufA ) {
            obuf = bufA;
            buf = bufB;
        } else {
            obuf = bufB;
            buf = bufA;
        }
	bc = chunk( buf, &bufin, &sizin );
        count = count + bc;
        if ( buffeq( buf, obuf ) ) {
            do {
                bc = chunk( buf, &bufin, &sizin );
                count = count + bc;
            } while ( ( bc > 0 ) && buffeq( buf, obuf ) );
            printf("*\n");
        }
    }
    printf("%x\n",count);
#if 0
    exit(0);
    c=getc(stdin);
    while(! feof(stdin)){
        printf("%.2x",c);
        bc++;
        if ( !(bc % BYTES_PER_CLUSTER) ) {
            if ( bc % BYTES_PER_LINE )
                putc(' ',stdout);
            else
                putc('\n',stdout);
        }

        c=getc(stdin);
    }
    if ( bc % BYTES_PER_LINE )
        putc('\n',stdout);
#endif
}

int addResultFromText( DBResultFile* orf, char* path ) {
    int err;
    char* data;
    char* pos;
    char* tab;
    int ipos;
    unsigned int numc, numv, trials;
    unsigned int nsys;
    double error;
    //double mean, reliability, consensus;
    //char name[64];
    int fd;
    struct stat sb;
    unsigned int i;
    Result* r;
    char** names;	// char*[nsys]
    int toret = 0;
    bool print = true;
    
    fd = open( path, O_RDONLY );
    if ( fd == -1 ) {
	perror("open");
	return -1;
    }
    err = fstat( fd, &sb );
    if ( err != 0 ) {
	perror("fstat");
	toret = err;
	goto endclose;
    }
    data = (char*)mmap( NULL, sb.st_size, PROT_READ, MAP_FILE, fd, 0 );
    if ( data == (char*)-1 ) {
	perror("mmap");
	toret = -1;
	goto endclose;
    }

    err = sscanf( data, "%d candidates, %d voters, %d trial elections, +/- %lf confusion error, %d systems",
	&numc, &numv, &trials, &error, &nsys );
    if ( nsys != orf->numSystems() ) {
	fprintf( stderr, "file has %d systems, expected %d\n", nsys, orf->numSystems() );
	toret = -1;
	goto endmunmap;
    }
    r = newResult( nsys );
    r->trials = trials;
    names = (char**)malloc( sizeof(char*) * nsys );
    //printf("fscanf err = %d\n", err );
    if ( print ) printf("%d candidates, %d voters, %d trial elections, +/- %lf confusion error, %d systems\n",
	numc, numv, trials, error, nsys );
    pos = data;
    ipos = 0;
    for ( i = 0; i < nsys; i++ ) {
	pos = (char*)memchr( pos, '\n', sb.st_size - ipos );
	pos++;
	ipos = data - pos;
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	{
	    int nlen;
	    nlen = tab - pos;
	    names[i] = (char*)malloc( nlen + 1 );
	    memcpy( names[i], pos, nlen );
	    names[i][nlen] = '\0';
	}
	pos = tab + 1;
	ipos = data - pos;
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	pos = tab + 1;
	ipos = data - pos;
	err = sscanf( pos, "%lf", &(r->systems[i].meanHappiness) );
	//printf("sscanf err = %d\n", err );
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	pos = tab + 1;
	ipos = data - pos;
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	pos = tab + 1;
	ipos = data - pos;
	err = sscanf( pos, "%lf", &(r->systems[i].reliabilityStd) );
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	pos = tab + 1;
	ipos = data - pos;
	tab = (char*)memchr( pos, '\t', sb.st_size - ipos );
	pos = tab + 1;
	ipos = data - pos;
	err = sscanf( pos, "%lf", &(r->systems[i].consensusStdAvg) );
	if ( print ) printf("%s\tmean happiness\t%.9lg\tstd\t%.9lg\tavg std\t%.9lg\n", names[i], r->systems[i].meanHappiness, r->systems[i].reliabilityStd, r->systems[i].consensusStdAvg );
    }

    for ( i = 0; i < nsys; i++ ) {
	if ( strcmp( names[i], orf->names.names[i] ) ) {
	    fprintf( stderr, "archive name[%d] \"%s\" != loaded name \"%s\"\n", i, orf->names.names[i], names[i] );
	    toret = -1;
	}
    }
    mergeResult( orf->get( numv, numc, error ), r, nsys );
    for ( i = 0; i < nsys; i++ ) {
	free( names[i] );
    }
    free( names );
    free( r );
endmunmap:
    munmap( data, sb.st_size );
endclose:
    close( fd );

    return toret;
}

const char* optstring = "a:c:e:F:lpv:nN:";

int main( int argc, char** argv ) {
    DBResultFile* orf = NULL;
    int err;
    int c;
    extern char* optarg;
    int numc = -1, numv = -1;
    double error = -1;

    while ( (c = getopt(argc,argv,optstring)) != -1 ) switch ( c ) {
    case 'a':	// add <file>
	addResultFromText( orf, optarg );
	break;
    case 'c':
	numc = atoi( optarg );
	break;
    case 'e':
	error = atof( optarg );
	break;
    case 'F':
	orf = DBResultFile::open( optarg, O_RDWR );
	if ( orf == NULL ) {
	orf = DBResultFile::open( optarg, O_RDONLY );
	if ( orf == NULL ) exit(1);
	}
	break;
    case 'p': {
	Result* r = orf->get( numv, numc, error );
	if ( r ) {
	    printResultHeader( stdout, numc, numv, r->trials, error, orf->numSystems() );
	    printResult( stdout, r, orf->names.names, orf->numSystems() );
	}
    }
	break;
    case 'l': {
	DBT k, d;
#if DB4
	DBC* curs;

	err = orf->db->cursor( orf->db, NULL, &curs, 0 );
	if ( err == 0 ) {
	    err = curs->c_get( curs, &k, &d, DB_FIRST );
	}
	while ( err == 0 ) {
	    ResultKeyStruct rks;
//	    printf("%0.8x\t%0.8x\tksize %d\tdsize %d\n", k.data, d.data, k.size, d.size );
	    if ( k.size != sizeof(rks) ) {
		printf("bogus k.size %d\n", k.size );
		goto nextRks;
	    }
	    memcpy( &rks, k.data, sizeof(rks) );
	    //printf("%0.8x\t%0.8x\n", ((u_int32_t*)k.data)[0], ((u_int32_t*)k.data)[1] );
	    //hexprint( k.data, k.size );
	    if ( ((u_int32_t*)&rks)[0] <= 0x00ffffff ) {
		Result* r = (Result*)d.data;
//		printf("r %p\t rks %p\n", r, rks );
		printResultHeader( stdout, rks.choices, rks.voters, r->trials, rks.error, orf->numSystems() );
//		printResult( stdout, r, orf->names.names, orf->numSystems() );
	    }
	    nextRks:
	    err = curs->c_get( curs, &k, &d, DB_NEXT );
	}
	curs->c_close( curs );
#else
	err = orf->db->seq( orf->db, &k, &d, R_FIRST );
	while ( err == 0 ) {
	    ResultKeyStruct rks;
//	    printf("%0.8x\t%0.8x\tksize %d\tdsize %d\n", k.data, d.data, k.size, d.size );
	    if ( k.size != sizeof(rks) ) {
		printf("bogus k.size %d\n", k.size );
		goto nextRks;
	    }
	    memcpy( &rks, k.data, sizeof(rks) );
	    //printf("%0.8x\t%0.8x\n", ((u_int32_t*)k.data)[0], ((u_int32_t*)k.data)[1] );
	    //hexprint( k.data, k.size );
	    if ( ((u_int32_t*)&rks)[0] <= 0x00ffffff ) {
		Result* r = (Result*)d.data;
//		printf("r %p\t rks %p\n", r, rks );
		printResultHeader( stdout, rks.choices, rks.voters, r->trials, rks.error, orf->numSystems() );
//		printResult( stdout, r, orf->names.names, orf->numSystems() );
	    }
	    nextRks:
	    err = orf->db->seq( orf->db, &k, &d, R_NEXT );
	}
#endif
    }
	break;
    case 'v':
	numv = atoi( optarg );
	break;
    case 'n': // print names
	fprintf(stderr,"%d names\n",orf->names.nnames);
	for ( int i = 0; i < orf->names.nnames; i++ ) {
	    printf("%s\n", orf->names.names[i] );
	}
	break;
    case 'N': { // set names from file
	    NameBlock newnames;
	    int fd;
	    int i;
	    struct stat s;
	    printf("reading names from file \"%s\"\n", optarg );
	    fd = open( optarg, O_RDONLY );
	    if ( fd < 0 ) { perror( argv[0] ); exit(1); }
	    err = fstat( fd, &s );
	    if ( err < 0 ) { perror( "fstat" ); exit(1); }
	    newnames.block = (char*)malloc( s.st_size + 2 );
	    err = read( fd, newnames.block, s.st_size );
	    if ( err < s.st_size ) { perror( "read" ); exit(1); }
	    close( fd );
	    for ( i = 0; i < s.st_size; i++ ) {
		if ( newnames.block[i] == '\n' ) {
		    newnames.block[i] = '\0';
		}
	    }
	    if ( newnames.block[s.st_size - 1] != '\0' ) {
		s.st_size++;
		newnames.block[s.st_size - 1] = '\0';
	    }
	    s.st_size++;
	    newnames.block[s.st_size - 1] = '\0';
	    newnames.blockLen = s.st_size;
	    parseNameBlock( &newnames );
	    fprintf(stderr,"%d names\n",newnames.nnames);
	    for ( int i = 0; i < newnames.nnames; i++ ) {
		printf("%s\n", newnames.names[i] );
	    }
	    orf->names.block = NULL;
	    orf->useNames( &newnames );
	}
	break;
    default:
	printf("unknown option \'%c\'\n", c );
    }
    orf->close();
    return 0;
}
