#ifndef NAME_BLOCK_H
#define NAME_BLOCK_H

class NameBlock {
public:
    char* block;
    int blockLen;
    char** names;
    int nnames;
	
	NameBlock();
	~NameBlock();
	// takes ownership of data, bulids names.
	// will free(data) on delete.
	bool parse(char* data, unsigned int len);
	// takes ownership of nameList, builds block.
	// will free(nameList) on delete.
	bool setNames(char** nameList, int numNames);
	
	bool copy(const NameBlock& a);
	
	NameBlock* clone() {
		NameBlock* nb = new NameBlock();
		nb->copy(*this);
		return nb;
	}
};

#endif
