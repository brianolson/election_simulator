#!/usr/bin/perl -w

$name = shift;

if ( ! defined $name ) {
	print STDERR "usage:\n\tnewMethod.pl methodName\n";
	exit 1;
}
if ( -e "${name}.cpp" ) {
	print STDERR "error: ${name}.cpp already exists\n";
	exit 1;
}
if ( -e "${name}.h" ) {
	print STDERR "error: ${name}.h already exists\n";
	exit 1;
}

$ucname = uc( $name );

open FOUT, '>', "${name}.cpp";
print FOUT<<EOF;
#include "Voter.h"
#include "${name}.h"

#if 0
void ${name}::init( const char** envp ) {
	const char* cur = *envp;
	while (cur != NULL) {
		if (0 == strncmp(cur, "seats=", 6)) {
			seats = strtol(cur + 6, NULL, 10);
		}
		envp++;
		cur = *envp;
	}
}
#endif

void ${name}::runElection( int* winnerR, const VoterArray& they ) const {
    int i;
    int* tally;
    int winner;
    int numc = they.numc;
    int numv = they.numv;

    // init things
    tally = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        tally[i] = 0;
    }

    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        tally[they[i].getMax()]++;
    }
    // find winner
    {
        int m = tally[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( tally[i] > m ) {
                m = tally[i];
                winner = i;
            }
        }
    }
    delete [] tally;
    if ( winnerR ) *winnerR = winner;
}

#if 0
bool ${name}::runMultiSeatElection( int* winnerR, const VoterArray& they, int seats ) const {
	if (seats == 1) {
		runElection(winnerR, they);
		return true;
	}
	return false;
}
#endif

VotingSystem* new${name}( const char* n ) {
	return new ${name}();
}
VSFactory* ${name}_f = new VSFactory( new${name}, "${name}" );

${name}::~${name}() {
}
EOF
close FOUT;

open FOUT, '>', "${name}.h";
print FOUT<<EOF;
#ifndef ${ucname}_H
#define ${ucname}_H

#include "VotingSystem.h"

class ${name} : public VotingSystem {
public:
	${name}() : VotingSystem( "${name}" ) {};
	//virtual void init( const char** envp );
	virtual void runElection( int* winnerR, const VoterArray& they ) const;
	//virtual bool runMultiSeatElection( int* winnerArray, const VoterArray& they, int seats ) const;
	virtual ~${name}();
};

#endif
EOF
close FOUT;
