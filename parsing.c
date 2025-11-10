
/* compile commands
linux: cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
windows: cc -std=c99 -Wall parsing.c mpc.c -o parsing
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mpc.h"
//#include "lval.h"
//#include "builtin.h"

// If compiling on windows
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}
// Fake add_history function
void add_history(char* unused) {}

// Else, include editline headers
#else
#include <editline/readline.h>
// mac X does not have history file
//#include <editline/history.h>
#endif


// forward delcarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

char* ltype_name(int type);

lval* lval_err(char* fmt, ...);

lval* builtin_op(lenv* e, lval* a, char* operation);

//lval* builtin(lenv* env, lval* arg, char* func);

lval* builtin_list(lenv* env, lval* arg);
lval* builtin_head(lenv* env, lval* arg); 
lval* builtin_tail(lenv* env, lval* arg); 
lval* builtin_join(lenv* env, lval* arg); 
lval* builtin_eval(lenv* env, lval* arg);

// lbuiltin function pointer
typedef lval*(*lbuiltin)(lenv*, lval*);

lval* lval_eval(lenv* e, lval* v);
lval* lval_take(lval* v, int i);
lval* lval_pop(lval* v, int i);
void lval_del(lval* v);
void lval_print(lval* v);


lenv* lenv_new(void);
lenv* lenv_copy(lenv* env);
void lenv_del(lenv* env);

// lisp value
struct lval {
	int type;
	
	// Basic
	long num;
	char* err;
	char* sym;

	// Function
	lbuiltin builtin;
	lenv* env;
	lval* formals;
	lval* body;
	
	// Expressions
	int count;
	// A list of lval pointers with its count
	struct lval** cell;
};
// environment structure
struct lenv {
	lenv* par;
	int count;
	// A list of symbols
	char** syms;
	// A list of values
	lval** vals;
};

// Error condition
#define LASSERT(args, cond, format_text, ...) \
	if (!(cond)) { \
		lval* err = lval_err(format_text, ##__VA_ARGS__); \
		lval_del(args); \
		return err; \
	}

#define LASSERT_ARGS(args, got, expected, func) { \
	bool cond = got == expected; \
	LASSERT(args, cond, "'%s' has too many arguments. " \
		"Got %i, Expected %i", func, got, expected); \
}

#define LASSERT_TYPE(args, got, expected, func) { \
	bool cond = got == expected; \
	LASSERT(args, cond, "'%s' passed the incorrect type. " \
		"Got %s, Expected %s", \
		func, ltype_name(got), ltype_name(expected)); \
}
/*
#define LASSERT_COUNT(args, got, expected, func) { \
	bool cond = got == expected; \
	LASSERT(args, cond, "'%s' . " \
		"Got is, Expected %i", \
		func, got, expected); \
}
*/


// lisp value types
enum {
	LVAL_ERR,
	LVAL_NUM,
	LVAL_SYM,
	LVAL_FUNC,
	LVAL_SEXPR,
	LVAL_QEXPR
};
// lisp value error types
enum {
	LERR_DIV_ZERO,
	LERR_BAD_OP,
	LERR_BAD_NUM
};

// LVAL TYPES CONSTRUCTORS

/*
Constructor for number lval pointer.
Converts long to lval number
*/
lval* lval_num(long num) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = num;
	return v;
}
/*
Constructor for error lval pointer.
Converts int long to lval error.
*/
lval* lval_err(char* fmt, ...) {
	// Alocate memory for lval pointer
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;

	// Create and initialize va list
	va_list va;
	va_start(va, fmt);

	// Allocate 512 bytes for error
	v->err = malloc(512);

	// printf the error with a maximum of 511 characters
	vsnprintf(v->err, 511, fmt, va);

	// Reallocate to used bytes
	v->err = realloc(v->err, strlen(v->err) + 1);

	// Cleanup va list
	va_end(va);
	return v;
}
/*
Constructor for symbol lval pointer
Converts a string symbol to a lval symbol
*/
lval* lval_sym(char* s) {
	// Alocate memory for lval pointer
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	// Copies the string s to v->sym
	strcpy(v->sym, s);
	return v;
}
/*
Constructor for s-expression lval
*/
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
/*
Constructor for q-expression lval
*/
lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
lval* lval_func(lbuiltin builtin) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUNC;
	v->builtin = builtin;
	return v;
}
// User-defined function
lval* lval_lambda(lval* formals, lval* body) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUNC;

	// Set builtin to null since function is user-defined
	v->builtin = NULL;

	// Create new environment 
	v->env = lenv_new();

	// Set formals and body
	v->formals = formals;
	v->body = body;
	return v;
}


lval* lval_copy(lval* v) {
	lval* copy = malloc(sizeof(lval));
	copy->type = v->type;
	
	switch (v->type) {
		case LVAL_FUNC: 
			// If a builtin funciton
			if (v->builtin) {
				copy->builtin = v->builtin; 
			}
			// If a user-defined function
			else {
				copy->builtin = NULL;
				copy->env = lenv_copy(v->env);
				copy->formals = lval_copy(v->formals);
				copy->body = lval_copy(v->body);
			}
			break;
		
		case LVAL_NUM: copy->num = v->num; break;
		case LVAL_ERR:
			// Allocate memory
			copy->err = malloc(strlen(v->err) + 1);
			// Copy v error message to the copy's
			strcpy(copy->err, v->err);
			break;
		case LVAL_SYM:
			// Allocate memory
			copy->sym = malloc(strlen(v->sym) + 1);
			// Copy v symbol to the copy's
			strcpy(copy->sym, v->sym);
			break;
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			copy->count = v->count;
			copy->cell = malloc(sizeof(lval*) * copy->count);
			
			for (int i = 0; i < copy->count; i++) {
				copy->cell[i] = lval_copy(v->cell[i]);
			}
			break;
	}
	return copy;
}

// Free up memory from lval pointer
void lval_del(lval* v) {
	switch (v->type) {
		// Do nothing for numbers and builtin functions
		case LVAL_NUM: break;
		case LVAL_FUNC: 
			// If the function is user-defined
			if (!v->builtin) {
				lenv_del(v->env);
				lval_del(v->formals);
				lval_del(v->body);
			}
			break;
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			// Free all elements in s expression
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			// And the container of the pointer
			free(v->cell);
			break;
	}
	free(v);
}
// ENVIRONMENT functions
// Create a new environment
lenv* lenv_new(void) {
	lenv* env = malloc(sizeof(lenv));
	env->par = NULL;
	env->count = 0;
	env->syms = NULL;
	env->vals = NULL;
	return env;
}

lenv* lenv_copy(lenv* env) {
	lenv* copy = malloc(sizeof(lenv));

	copy->count = env->count;
	copy->syms = env->syms;
	copy->vals = env->vals;

	lenv_del(env);
	return copy;
}

lval* lenv_get(lenv* e, lval* k) {
	for (int i = 0; i < e->count; i++) {
		// If the stored symbol string matches k's symbol string
		if (strcmp(e->syms[i], k->sym) == 0) {
			// Return a copy of the value
			return lval_copy(e->vals[i]);
		}
	}
	// If no symbol was found return error
	return lval_err("Unbound symbol! %s", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v) {
	// Check entire environment for duplicate variable
	for (int i = 0; i < e->count; i++) {
		// If variable is already in environment
		if (strcmp(e->syms[i], k->sym) == 0) {
			// Delete found variable
			lval_del(e->vals[i]);
			// Replace with v variable
			e->vals[i] = lval_copy(v);
			return;
		}
	}
	// If it does not exsist, create new entry
	e->count++;
	// Allocate memory for new entry
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);
	
	// Copy lval and symbol into environment
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym) + 1);
	strcpy(e->syms[e->count-1], k->sym);
}

void lenv_del(lenv* e) {
	for (int i = 0; i < e->count; i++) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}


// Adds an lval element to a cell
lval* lval_add(lval* v, lval* x) {
	v->count++;
	// Allocate extra space for new lval.
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}
lval* lval_read_num(mpc_ast_t* tree) {
	lval* v;
	errno = 0;
	long x = strtol(tree->contents, NULL, 10);
	// If not invalid number
	if (errno != ERANGE) {
		v = lval_num(x);
	} else {
		v = lval_err("invalid number");
	}
	return v;
}
lval* lval_read(mpc_ast_t* tree) {
	if (strstr(tree->tag, "number")) { return lval_read_num(tree); }
	if (strstr(tree->tag, "symbol")) { return lval_sym(tree->contents); }
	
	// If root (>) or sexpr then create empty list
	lval* result = NULL;
	if (strcmp(tree->tag, ">") == 0) { result = lval_sexpr(); }
	if (strstr(tree->tag, "sexpr")) { result = lval_sexpr(); }
	if (strstr(tree->tag, "qexpr")) { result = lval_qexpr(); }
	
	// Fill list with any valid expression contained within
	for (int i = 0; i < tree->children_num; i++) {
		// Ignore brackets, and regex
		if (strcmp(tree->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, "{") == 0) { continue; }
		if (strcmp(tree->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(tree->children[i]->tag, "regex") == 0) { continue; }
		result = lval_add(result, lval_read(tree->children[i]));
	}
	return result;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
	// Evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(e, v->cell[i]);
	}
	// Check for errors
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take(v,i);
		}
	}
	// If expression is empty
	if (v->count == 0) { return v; }
	// If expression is single
	if (v->count == 1) { return lval_take(v,0); }
	
	// Check if first element is a function
	lval* first = lval_pop(v, 0);
	// If first element is not a function, give an error
	if (first->type != LVAL_FUNC) {
		lval_del(v);
		lval_del(first);
		return lval_err("First element is not a function.");
	}
	// If first element is a funciton, call it and get result
	lval* result = first->builtin(e, v);
	lval_del(first);
	return result;
}

lval* lval_eval(lenv* e, lval* v) {
	if (v->type == LVAL_SYM) {
		// Get a copy of v value
		lval* x = lenv_get(e, v);
		// Delete original
		lval_del(v);
		return x;
	}
	// Evaluate s-expression
	if (v->type == LVAL_SEXPR) { 
		return lval_eval_sexpr(e, v); 
	}
	// Evaluate others directly
	return v;
}

// Pops a lval from a s-expression
lval* lval_pop(lval* v, int i) {
	lval* popped_lval = v->cell[i];
	
	// Decrease amount of items in s-expression
	v->count--;
	
	// Shift memory after the item at i over the top
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i));
	
	// Reallocate memory
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	
	return popped_lval;
}
// Takes a lval from a s-expression and deletes the s-expression
lval* lval_take(lval* v, int i) {
	lval* result = lval_pop(v, i);
	lval_del(v);
	return result;
}
// Moves all lvals from y to x
lval* lval_join(lval* x, lval* y) {
	// Each cell in y is added to x
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}
	lval_del(y);
	return x;
}

char* ltype_name(int type) {
	switch (type) {
		case LVAL_FUNC: return "Function";
		case LVAL_NUM: return "Number";
		case LVAL_ERR: return "Error";
		case LVAL_SYM: return "Symbol";
		case LVAL_SEXPR: return "S-Expression";
		case LVAL_QEXPR: return "Q-Expression";
		default: return "Unknown";
	}
}

bool isAddition(char* operatation) {
	return strcmp(operatation, "+") == 0;
}
bool isSubtraction(char* operatation) {
	return strcmp(operatation, "-") == 0;
}
bool isMultiplication(char* operatation) {
	return strcmp(operatation, "*") == 0;
}
bool isDivision(char* operatation) {
	return strcmp(operatation, "/") == 0;
}

lval* builtin_op(lenv* e, lval* a, char* operation) {
	// Check if all aruments are numbers
	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE(a, a->cell[i]->type, LVAL_NUM, operation);
	}
	// Get the first element
	lval* x = lval_pop(a, 0);
	
	// If there are no other elements and operator is -
	if (a->count == 0 && isSubtraction(operation)) {
		// negate number
		x->num *= -1;
	}
	
	while (a->count > 0) {
		lval* y = lval_pop(a, 0);
		if (isAddition(operation)) { x->num += y->num ; }
		if (isSubtraction(operation)) { x->num  -= y->num ; }
		if (isMultiplication(operation)) { x->num *= y->num; }
		if (isDivision(operation)) { 
			// if denominator is 0
			if (y->num == 0) {
				
				lval_del(x);
				lval_del(y);
				x = lval_err("Can't divide by zero");
				break;
			} else {
				x->num /= y->num;
			}
		}
		lval_del(y);
	}
	return x;
}
lval* builtin_add(lenv* env, lval* arg) {
	return builtin_op(env, arg, "+");
}
lval* builtin_sub(lenv* env, lval* arg) {
	return builtin_op(env, arg, "-");
}
lval* builtin_mul(lenv* env, lval* arg) {
	return builtin_op(env, arg, "*");
}
lval* builtin_div(lenv* env, lval* arg) {
	return builtin_op(env, arg, "/");
}
/*
lval* builtin_mod(lenv* env, lval* arg) {
	return builtin_op(env, arg, "");
}
*/
lval* builtin_head(lenv* e, lval* arg) {
	// Error checking conditions
	LASSERT_ARGS(arg, arg->count, 1, "head");
	LASSERT_TYPE(arg, arg->cell[0]->type, LVAL_QEXPR, "head");
	
	LASSERT(arg, arg->cell[0]->count != 0,
		"'head' function passed {}!");
	
	// Else, take first arguement / head
	lval* v = lval_take(arg, 0);
	// Delete everything except for head and return
	while (v->count > 1) {
		lval_del(lval_pop(v,1));
	}
	return v;
}
lval* builtin_tail(lenv* e, lval* arg) {
	// Error checking conditions
	LASSERT_ARGS(arg, arg->count, 1, "tail");
	LASSERT_TYPE(arg, arg->cell[0]->type, LVAL_QEXPR, "tail");
	LASSERT(arg, arg->cell[0]->count != 0,
		"'tail' function passed {}!");
	
	// Else, take first arguement
	lval* v = lval_take(arg, 0);
	// Delete first element and return
	lval_del(lval_pop(v, 0));
	return v;
}
// Converts a q-expression to a s-expression
lval* builtin_list(lenv* e, lval* arg) {
	arg->type = LVAL_QEXPR;
	return arg;
}
// Converts a q-expression to a s-expression and evaluates it
lval* builtin_eval(lenv* e, lval* arg) {
	LASSERT_ARGS(arg, arg->count, 1, "eval");
	LASSERT_TYPE(arg, arg->cell[0]->type, LVAL_QEXPR, "eval");
	// Take first arguement and convert to s-expression
	lval* x = lval_take(arg, 0);
	x->type = LVAL_SEXPR;
	// Evaluate
	lval* result = lval_eval(e, x);
	return result;
}

lval* builtin_join(lenv* e, lval* arg) {
	// Check that all arguments are q-expressions
	for (int i = 0; i < arg->count; i++) {
		LASSERT_TYPE(arg, arg->cell[i]->type, LVAL_QEXPR, "join");
	}
	lval* x = lval_pop(arg, 0);
	
	// Do until the arg is empty
	while (arg->count) {
		// Pop the first value from arg and put them into x 
		lval* y = lval_pop(arg, 0);
		x = lval_join(x, y);
	}
	lval_del(arg);
	return x;
}
lval* builtin_def(lenv* env, lval* arg) {
	// Error when not a q-expression
	LASSERT_TYPE(arg, arg->cell[0]->type, LVAL_QEXPR, "join");
	
	// First argument is symbol list
	lval* syms = arg->cell[0];

	// Check that all elements of first list are symbols
	for (int i = 0; i < syms->count; i++) {
		LASSERT(arg, syms->cell[i]->type == LVAL_SYM, 
			"'def' function cannot define non-symbol");
	}

	// Check that there is correct amount of symbols and values
	LASSERT(arg, syms->count == arg->count-1, 
		"'def' function cannot define incorrect number of values to symbols");
	
	// Put copies of values to symbols
	for (int i = 0; i < syms->count; i++) {
		lenv_put(env, syms->cell[i], arg->cell[i+1]);
	}
	lval_del(arg);
	return lval_sexpr();
}

lval* builtin_lambda(lenv* env, lval* arg) {
	// Check for 2 arguements that are both q-expressions
	LASSERT_ARGS(arg, arg->count, 2, "\\");
	LASSERT_TYPE(arg, arg->cell[0]->type, LVAL_QEXPR, "\\");
	LASSERT_TYPE(arg, arg->cell[1]->type, LVAL_QEXPR, "\\");

	// Check that q-expression only has symbols
	for (int i = 0; i < arg->cell[0]->count; i++) {
		LASSERT(arg, (arg->cell[0]->cell[i]->type == LVAL_SYM),
				"Cannot define non-symbol. Got %s, Expected %s",
			ltype_name(arg->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
	}
	// Get first two arguements 
	lval* formals = lval_pop(arg, 0);
	lval* body = lval_pop(arg, 0);
	lval_del(arg);

	// Return user-defined funciton
	return lval_lambda(formals, body);
}

void lenv_builtin_add(lenv* env, char* name, lbuiltin func) {
	lval* k = lval_sym(name);
	lval* v = lval_func(func);
	// Put copies into environment
	lenv_put(env, k, v);
	// Delete originals
	lval_del(k);
	lval_del(v);
}
void lenv_add_builtins(lenv* e) {
	// variable functions
	lenv_builtin_add(e, "def", builtin_def);
	lenv_builtin_add(e, "\\", builtin_lambda);

	// list functions
	lenv_builtin_add(e, "list", builtin_list);
	lenv_builtin_add(e, "head", builtin_head);
	lenv_builtin_add(e, "tail", builtin_tail);
	lenv_builtin_add(e, "eval", builtin_eval);
	lenv_builtin_add(e, "join", builtin_join);
	
	// math functions
	lenv_builtin_add(e, "+", builtin_add);
	lenv_builtin_add(e, "-", builtin_sub);
	lenv_builtin_add(e, "*", builtin_mul);
	lenv_builtin_add(e, "/", builtin_div);
	lenv_builtin_add(e, "add", builtin_add);
	lenv_builtin_add(e, "sub", builtin_sub);
	lenv_builtin_add(e, "mul", builtin_mul);
	lenv_builtin_add(e, "div", builtin_div);
}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		// Print value inside
		lval_print(v->cell[i]);
		// Put whitespace for all except for last element
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

// prints value or error of lisp value
void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_FUNC: 
			if (v->builtin) {
				printf("<function>");
			} else {
				printf("(\\ ");
				lval_print(v->formals);
				putchar(' ');
				lval_print(v->body);
				putchar(')');
			}
		 	break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
	}
}
// print lisp value with newline
void lval_println(lval* v) {
	lval_print(v);
	putchar('\n');
}


int main(int argc, char** argv) {
	// Parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Qexpr = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	const char* language = "                               \
		number   : /-?[0-9]+/;                             \
		symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
		sexpr : '(' <expr>* ')';                    \
		qexpr : '{' <expr>* '}';                    \
		expr     : <number> | <symbol> | <sexpr> | <qexpr>;  \
		lispy    : /^/ <expr>+ /$/ ;             \
	  ";  

	// Define the language
	mpca_lang(MPCA_LANG_DEFAULT, language,
	  Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	  
	lenv* env = lenv_new();
	lenv_add_builtins(env);
	
	puts("Lispy Version 0.0.1");
	puts("Press Ctrl+c to Exit\n");
	
	// Infinite loop until Ctrl+c
	while (1) {
		// Get user input
		char* input = readline("lispee> ");
		// Add command history
		add_history(input);
		
		// Try to parse user input
		mpc_result_t result;
		
		// If parsing sucessful
		if (mpc_parse("<stdin>", input, Lispy, &result)) {
			lval* x = lval_eval(env, lval_read(result.output));
			lval_println(x);
			lval_del(x);
			
			mpc_ast_delete(result.output);
		} else {
			// Else, print error
			mpc_err_print(result.error);
			mpc_err_delete(result.error);
		}
		free(input);
	}
	lenv_del(env);
	
	// Undefine and delete parsers 
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	
	return 0;
}