
#ifndef lval_h
#define lval_h

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


lval* lval_eval(lenv* e, lval* v);
lval* lval_take(lval* v, int i);
lval* lval_pop(lval* v, int i);


void lval_print(lval* v);

#endif