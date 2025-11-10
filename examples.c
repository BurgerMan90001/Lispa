
/*
// Adjective parser
mpc_parser_t* Adjective = mpc_or(4,
	mpc_sym("wow"), mpc_sym("many"),
	mpc_sym("so"), mpc_sym("such"),
);
// Noun parser
mpc_parser_t* Noun = mpc_or(5,
	mpc_sym("lisp"), mpc_sym("language"),
	mpc_sym("book"), mpc_sym("build"),
	mpc_sym("c")
);

mpc_parser_t* Phrase = mpc_and(2, mpcf_strfold, 
	Adjective, Noun, free);

mpc_parser_t* Doge = mpc_many(mpcf_strfold, Phrase);

// Create rules
mpc_parser_t* Adjective = mpc_new("adjective");
mpc_parser_t* Noun = mpc_new("noun");
mpc_parser_t* Phrase = mpc_new("phrase");
mpc_parser_t* Doge = mpc_new("doge");

// Define rules
mpca_lang(MPCA_LANG_DEFAULT,
	"                                         \
    adjective : \"wow\" | \"many\"            \
              |  \"so\" | \"such\";           \
    noun      : \"lisp\" | \"language\"       \
              | \"book\" | \"build\" | \"c\"; \
    phrase    : <adjective> <noun>;           \
    doge      : <phrase>*;                    \
	",
	Adjective, Noun, Phrase, Doge);
	

mpc_cleanup(4, Adjective, Noun, Phrase, Doge);

// counts the number of nodes in a tree
int number_of_nodes(mpc_ast_t* tree) {
	// base case no children
	if (tree->children_num == 0) {
		return 1;
	}
	if (tree->children_num >= 1) {
		int total = 1;
		for (int i = 0; i < tree->children_num; i++) {
			total = total + number_of_nodes(tree->children[i]);
		}
	}
	return 0;
}
*/

/*
lval* builtin(lenv* e, lval* arg, char* name) {
	if (strcmp("list", name) == 0) { return builtin_list(e, arg); }
	if (strcmp("head", name) == 0) { return builtin_head(e, arg); }
	if (strcmp("tail", name) == 0) { return builtin_tail(e, arg); }
	if (strcmp("join", name) == 0) { return builtin_join(e, arg); }
	if (strcmp("eval", name) == 0) { return builtin_eval(e, arg); }
	if (strstr("+-/*", name)) { return builtin_op(e, arg, name); }
	lval_del(arg);
	return lval_err("Unknown function");
}
*/