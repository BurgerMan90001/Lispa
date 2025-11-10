
#include "lval.h"



#include "builtin.h"

// lisp value
struct lval {
	int type;
	
	long num;
	// Error and symbol type string data
	char* err;
	char* sym;
	lbuiltin func;
	
	// a list of lval pointers with its count
	int count;
	struct lval** cell;
};
// environment structure
struct lenv {
	int count;
	// A list of symbols
	char** syms;
	// A list of values
	lval** vals;
};

