#!/usr/bin/perl -w

$name = shift;

if ( ! defined $name ) {
	print STDERR "usage:\n\tnewMethod.pl methodName\n";
}
if ( -e "${name}.cpp" ) {
	print STDERR "error: ${name}.cpp already exists\n";
}
if ( -e "${name}.h" ) {
	print STDERR "error: ${name}.h already exists\n";
}

$ucname = uc( $name );

open FOUT, '>', "${name}.cpp";
print FOUT<<EOF;
#include "Voter.h"
#include "${name}.h"

void ${name}::runElection( int* winnerR, const VoterArray& they ) {
    int i;
    int* talley;
    int winner;
    int numc = they.numc;
    int numv = they.numv;
    
    // init things
    talley = new int[numc];
    for ( i = 0; i  < numc; i++ ) {
        talley[i] = 0;
    }
    
    // count votes for each candidate
    for ( i = 0; i < numv; i++ ) {
        talley[they[i].getMax()]++;
    }
    // find winner
    {
        int m = talley[0];
        winner = 0;
        for ( i = 1; i < numc; i++ ) {
            if ( talley[i] > m ) {
                m = talley[i];
                winner = i;
            }
        }
    }
    delete [] talley;
    if ( winnerR ) *winnerR = winner;
}

VotingSystem* new${name}( char* n ) {
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
    virtual void runElection( int* winnerR, const VoterArray& they );
    virtual ~${name}();
};

#endif
EOF
close FOUT;
