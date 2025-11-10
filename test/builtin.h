
#include "lval.h"


lval* builtin_op(lenv* e, lval* a, char* operation);

lval* builtin(lenv* env, lval* arg, char* func);


lval* builtin_list(lenv* env, lval* arg);
lval* builtin_head(lenv* env, lval* arg); 
lval* builtin_tail(lenv* env, lval* arg); 
lval* builtin_join(lenv* env, lval* arg); 
lval* builtin_eval(lenv* env, lval* arg);

// lbuiltin function pointer
typedef lval*(*lbuiltin)(lenv*, lval*);
