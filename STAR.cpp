#include "Voter.h"
#include "STAR.h"

#include <math.h>
#include <memory>
#include <utility>

using std::unique_ptr;

inline double quantize(int quantization, double rating) {
	// e.g. for quantization=5 map [0..1] to [0,1,2,3,4,5]
	if (quantization <= 0) return rating;
	return floor(rating * (quantization + 1.0) * 0.999999999999999);
}

// https://www.equal.vote/starvoting
void STARVote::runElection( int* winnerR, const VoterArray& they ) const {
	// sum up raw scores
	unique_ptr<double[]> sums(new double[they.numc]);
	for (int c = 0; c < they.numc; c++) {
		sums[c] = 0.0;
	}
	for (int v = 0; v < they.numv; v++) {
		for (int c = 0; c < they.numc; c++) {
			sums[c] += quantize(quantization, they[v].getPref(c));
		}
	}

	// find top two
	int firsti;
	int secondi;
	double firstv;
	double secondv;
	if (sums[0] > sums[1]) {
		firsti = 0;
		secondi = 1;
	} else {
		firsti = 1;
		secondi = 0;
	}
	firstv = sums[firsti];
	secondv = sums[secondi];
	for (int c = 2; c < they.numc; c++) {
		if (sums[c] > secondv) {
			secondv = sums[c];
			secondi = c;
			if (secondv > firstv) {
				std::swap(secondv, firstv);
				std::swap(secondi, firsti);
			}
		}
	}

	// between top two, go through votes to cast vote for preferred one
	int firstc = 0;
	int secondc = 0;
	for (int v = 0; v < they.numv; v++) {
		double fv = quantize(quantization, they[v].getPref(firsti));
		double sv = quantize(quantization, they[v].getPref(secondi));
		if (fv > sv) {
			firstc++;
		} else if (sv > fv) {
			secondc++;
		} // else neither is preferred, no increment for either.
	}
	if (winnerR) {
		if (firstc >= secondc) {
			*winnerR = firsti;
		} else {
			*winnerR = secondi;
		}
	}
}

VotingSystem* newSTARVote( const char* n ) {
	return new STARVote();
}
VSFactory* STARVote_f = new VSFactory( newSTARVote, "STAR" );

STARVote::~STARVote() {
}
