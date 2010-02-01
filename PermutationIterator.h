#ifndef PERMUTATION_ITERATOR_H
#define PERMUTATION_ITERATOR_H

#include <assert.h>

class PermutationIterator {
public:
	PermutationIterator(int seats_, int choices_) : seats(seats_), choices(choices_) {
		assert(choices > seats);
		state = new int[seats];
		for (int i = 0; i < seats; ++i) {
			state[i] = i;
		}
	}
	~PermutationIterator() {
		delete [] state;
	}
	
	inline void get(int* out) {
		for (int i = 0; i < seats; ++i) {
			out[i] = state[i];
		}
	}
	bool increment() {
		int pos = seats - 1;
		while (true) {
			while (true) {
				assert(pos >= 0);
				assert(pos < seats);
				state[pos]++;
				if (state[pos] < choices) {
					pos++;
					if (pos >= seats) {
						return true;
					}
					break;
				}
				pos--;
				if (pos < 0) {
					return false;
				}
			}
			while (pos < seats) {
				if (pos <= 0) {
					return false;
				}
				assert(pos >= 1);
				assert(pos < seats);
				state[pos] = state[pos-1] + 1;
				if (state[pos] >= choices) {
					pos--;
					if (pos < 0) {
						return false;
					}
					break;
				}
				pos++;
				if (pos >= seats) {
					return true;
				}
			}
		}
	}
	
	inline bool next(int* out) {
		get(out);
		return increment();
	}
protected:
	int seats;
	int choices;
	int* state;
};

#endif /* PERMUTATION_ITERATOR_H */
