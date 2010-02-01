#include "PermutationIterator.h"

#include <stdio.h>

int main(int argc, char** argv) {
	PermutationIterator pi(3, 6);
	int set[3];
	do {
		pi.get(set);
		printf("%d %d %d\n", set[0], set[1], set[2]);
	} while(pi.increment());
}