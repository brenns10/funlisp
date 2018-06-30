/*
 * builtins.c: the built-in functions of funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>
#include <string.h>

#include "funlisp_internal.h"

static lisp_value *lisp_builtin_eval(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *arguments, void *user)
{
	(void) user; /* unused */
	return lisp_eval(rt, scope, arguments->left);
}

static lisp_value *lisp_builtin_car(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *arglist, void *user)
{
	lisp_list *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "l", &firstarg)) {
		return NULL;
	}
	if (lisp_nil_p((lisp_value*) firstarg)) {
		return lisp_error(rt, LE_ERROR, "car of nil list");
	}
	return firstarg->left;
}

static lisp_value *lisp_builtin_cdr(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *a, void *user)
{
	lisp_list *firstarg;
	lisp_list *arglist = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */

	lisp_error_check(arglist);
	if (!lisp_get_args(rt, arglist, "l", &firstarg)) {
		return NULL;
	}
	return firstarg->right;
}

static lisp_value *lisp_builtin_quote(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *arglist, void *user)
{
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;
	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return arglist->left;
}

static lisp_value *lisp_builtin_cons(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *a, void *user)
{
	lisp_value *a1;
	lisp_value *l;
	lisp_list *new, *arglist = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */
	lisp_error_check(arglist);
	if (!lisp_get_args(rt, arglist, "**", &a1, &l)) {
		return NULL;
	}
	new = (lisp_list*)lisp_new(rt, type_list);
	new->left = a1;
	new->right = (lisp_value*)l;
	return (lisp_value*)new;
}

static lisp_value *lisp_builtin_lambda(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *arguments, void *user)
{
	lisp_list *argnames, *code;
	lisp_list *it;
	lisp_lambda *lambda;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arguments, "lR", &argnames, &code)) {
		return NULL;
	}

	it = argnames;
	lisp_for_each(it) {
		if (it->left->type != type_symbol) {
			return lisp_error(rt, LE_TYPE, "argument names must be symbols");
		}
	}

	lambda = (lisp_lambda*)lisp_new(rt, type_lambda);
	lambda->args = argnames;
	lambda->code = code;
	lambda->closure = scope;
	lambda->lambda_type = TP_LAMBDA;
	return (lisp_value*) lambda;
}

static lisp_value *lisp_builtin_macro(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *arguments, void *user)
{
	lisp_list *argnames, *code;
	lisp_list *it;
	lisp_lambda *lambda;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arguments, "lR", &argnames, &code)) {
		return NULL;
	}

	it = argnames;
	lisp_for_each(it) {
		if (it->left->type != type_symbol) {
			return lisp_error(rt, LE_TYPE, "argument names must be symbols");
		}
	}

	lambda = (lisp_lambda*)lisp_new(rt, type_lambda);
	lambda->args = argnames;
	lambda->code = code;
	lambda->closure = scope;
	lambda->lambda_type = TP_MACRO;
	return (lisp_value*) lambda;
}

static lisp_value *lisp_builtin_define(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *a, void *user)
{
	lisp_symbol *s;
	lisp_value *expr, *evald;
	(void) user; /* unused */

	if (!lisp_get_args(rt, a, "s*", &s, &expr)) {
		return NULL;
	}

	evald = lisp_eval(rt, scope, expr);
	lisp_error_check(evald);
	lisp_scope_bind(scope, s, evald);
	return evald;
}

static lisp_value *lisp_builtin_plus(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *a, void *user)
{
	int sum = 0;
	lisp_integer *i;
	lisp_list *args = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */
	lisp_error_check(args);

	lisp_for_each(args) {
		if (args->left->type != type_integer) {
			return lisp_error(rt, LE_TYPE, "expect integers for addition");
		}
		i = (lisp_integer*) args->left;
		sum += i->x;
	}

	i = (lisp_integer*)lisp_new(rt, type_integer);
	i->x = sum;
	return (lisp_value*)i;
}

static lisp_value *lisp_builtin_minus(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *a, void *user)
{
	int val = 0, len;
	lisp_integer *i;
	lisp_list *args;
	(void) user; /* unused */

	args = lisp_eval_list(rt, scope, a);
	lisp_error_check(args);
	len = lisp_list_length(args);

	if (len < 1) {
		return lisp_error(rt, LE_2FEW, "expected at least one arg");
	} else if (len == 1) {
		i = (lisp_integer*) args->left;
		val = - i->x;
	} else {
		i = (lisp_integer*) args->left;
		val = i->x;
		args = (lisp_list*)args->right;
		lisp_for_each(args) {
			if (args->left->type != type_integer) {
				return lisp_error(rt, LE_TYPE, "expected integer");
			}
			i = (lisp_integer*) args->left;
			val -= i->x;
		}
	}

	i = (lisp_integer*)lisp_new(rt, type_integer);
	i->x = val;
	return (lisp_value*)i;
}

static lisp_value *lisp_builtin_multiply(lisp_runtime *rt, lisp_scope *scope,
                                         lisp_list *a, void *user)
{
	lisp_integer *i;
	int product = 1;
	lisp_list *args = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */
	lisp_error_check(args);

	lisp_for_each(args) {
		if (args->left->type != type_integer) {
			return lisp_error(rt, LE_TYPE, "expect integers for multiplication");
		}
		i = (lisp_integer*) args->left;
		product *= i->x;
	}

	i = (lisp_integer*)lisp_new(rt, type_integer);
	i->x = product;
	return (lisp_value*)i;
}

static lisp_value *lisp_builtin_divide(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *a, void *user)
{
	lisp_integer *i;
	int val = 0, len;
	lisp_list *args = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */

	lisp_error_check(args);
	len = lisp_list_length(args);

	if (len < 1) {
		return lisp_error(rt, LE_2FEW, "expected at least one arg");
	}
	i = (lisp_integer*) args->left;
	val = i->x;
	args = (lisp_list*)args->right;
	lisp_for_each(args) {
		if (args->left->type != type_integer) {
			return lisp_error(rt, LE_TYPE, "expected integer");
		}
		i = (lisp_integer*) args->left;
		if (i->x == 0) {
			return lisp_error(rt, LE_ERROR, "divide by zero");
		}
		val /= i->x;
	}

	i = (lisp_integer*)lisp_new(rt, type_integer);
	i->x = val;
	return (lisp_value*)i;
}

static lisp_value *lisp_builtin_cmp_util(lisp_runtime *rt, lisp_scope *scope,
                                         lisp_list *a, void *user)
{
	lisp_integer *first, *second, *result;
	lisp_list *args = lisp_eval_list(rt, scope, a);
	(void) user; /* unused */
	lisp_error_check(args);

	if (!lisp_get_args(rt, (lisp_list*)args, "dd", &first, &second)) {
		return NULL;
	}

	result = (lisp_integer*)lisp_new(rt, type_integer);
	result->x = first->x - second->x;
	return (lisp_value*)result;
}

static lisp_value *lisp_builtin_eq(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_integer *v = (lisp_integer*)lisp_builtin_cmp_util(rt, scope, a, user);
	(void) user; /* unused */
	lisp_error_check(v);
	if (v->type == type_integer) {
		v->x = (v->x == 0);
	}
	return (lisp_value*)v;
}

static lisp_value *lisp_builtin_gt(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_integer *v = (lisp_integer*)lisp_builtin_cmp_util(rt, scope, a, user);
	(void) user; /* unused */
	lisp_error_check(v);
	if (v->type == type_integer) {
		v->x = (v->x > 0);
	}
	return (lisp_value*)v;
}

static lisp_value *lisp_builtin_ge(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_integer *v = (lisp_integer*)lisp_builtin_cmp_util(rt, scope, a, user);
	(void) user; /* unused */
	lisp_error_check(v);
	if (v->type == type_integer) {
		v->x = (v->x >= 0);
	}
	return (lisp_value*)v;
}

static lisp_value *lisp_builtin_lt(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_integer *v = (lisp_integer*)lisp_builtin_cmp_util(rt, scope, a, user);
	(void) user; /* unused */
	lisp_error_check(v);
	if (v->type == type_integer) {
		v->x = (v->x < 0);
	}
	return (lisp_value*)v;
}

static lisp_value *lisp_builtin_le(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_integer *v = (lisp_integer*)lisp_builtin_cmp_util(rt, scope, a, user);
	(void) user; /* unused */
	lisp_error_check(v);

	if (v->type == type_integer) {
		v->x = (v->x <= 0);
	}
	return (lisp_value*)v;
}

static lisp_value *lisp_builtin_if(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	lisp_value *condition, *body_true, *body_false;
	(void) user; /* unused */

	if (!lisp_get_args(rt, a, "***", &condition, &body_true, &body_false)) {
		return NULL;
	}

	condition = lisp_eval(rt, scope, condition);
	lisp_error_check(condition);
	if (condition->type == type_integer && ((lisp_integer*)condition)->x) {
		return lisp_eval(rt, scope, body_true);
	} else {
		return lisp_eval(rt, scope, body_false);
	}
}

static lisp_value *lisp_builtin_null_p(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *a, void *user)
{
	lisp_integer *result;
	lisp_value *v;
	lisp_list *args;
	
	(void) user; /* unused */
	args = lisp_eval_list(rt, scope, a);
	lisp_error_check(args);

	if (!lisp_get_args(rt, args, "*", &v)) {
		return NULL;
	}

	result = (lisp_integer*) lisp_new(rt, type_integer);
	result->x = (int) lisp_nil_p(v);
	return (lisp_value*)result;
}

static lisp_list *get_quoted_left_items(lisp_runtime *rt, lisp_list *list_of_lists)
{
	lisp_list *left_items = NULL, *rv, *l;

	lisp_for_each(list_of_lists) {
		/* Create or advance left_items to the next list. */
		if (left_items == NULL) {
			left_items = (lisp_list*) lisp_new(rt, type_list);
			rv = left_items;
		} else {
			left_items->right = lisp_new(rt, type_list);
			left_items = (lisp_list*) left_items->right;
		}
		/* Check the next node in the list to make sure it's actually a list. */
		if (lisp_nil_p(list_of_lists->left)) {
			return NULL;
		}
		/* Get the next node in the list and get the argument. */
		l = (lisp_list*) list_of_lists->left;
		left_items->left = lisp_quote(rt, l->left);
	}
	left_items->right = lisp_nil_new(rt);
	return rv;
}

static lisp_list *advance_lists(lisp_runtime *rt, lisp_list *list_of_lists)
{
	lisp_list *right_items = NULL, *rv, *l;

	lisp_for_each(list_of_lists) {
		/* Create or advance left_items to the next list. */
		if (right_items == NULL) {
			right_items = (lisp_list*) lisp_new(rt, type_list);
			rv = right_items;
		} else {
			right_items->right = lisp_new(rt, type_list);
			right_items = (lisp_list*) right_items->right;
		}
		/* Check the next node in the list to make sure it's actually a list. */
		if (list_of_lists->left->type != type_list) {
			return NULL;
		}
		/* Get the next node in the list and get the argument. */
		l = (lisp_list*) list_of_lists->left;
		right_items->left = l->right;
	}
	right_items->right = lisp_nil_new(rt);
	return rv;
}

static lisp_value *lisp_builtin_map(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *a, void *user)
{
	lisp_value *f;
	lisp_list *ret = NULL, *args, *rv;
	lisp_list *map_args;
	(void) user; /* unused */

	map_args = lisp_eval_list(rt, scope, a);
	lisp_error_check(map_args);

	/* Get the function from the first argument in the list. */
	f = map_args->left;
	if (map_args->right->type != type_list) {
		return lisp_error(rt, LE_2FEW, "need at least two arguments");
	}
	map_args = (lisp_list*) map_args->right;
	while ((args = get_quoted_left_items(rt, map_args)) != NULL) {
		if (ret == NULL) {
			ret = (lisp_list*) lisp_new(rt, type_list);
			rv = ret;
		} else {
			ret->right = lisp_new(rt, type_list);
			ret = (lisp_list*) ret->right;
		}
		ret->left = lisp_call(rt, scope, f, (lisp_value*)args);
		lisp_error_check(ret->left);
		map_args = advance_lists(rt, map_args);
	}
	ret->right = lisp_nil_new(rt);
	return (lisp_value*) rv;
}

static lisp_list *lisp_new_pair_list(lisp_runtime *rt, lisp_value *one, lisp_value *two)
{
	lisp_list *first_node = (lisp_list*) lisp_new(rt, type_list);
	lisp_list *second_node = (lisp_list*) lisp_new(rt, type_list);
	first_node->left = one;
	first_node->right = (lisp_value*) second_node;
	second_node->left = two;
	second_node->right = lisp_nil_new(rt);
	return first_node;
}

static lisp_value *lisp_builtin_reduce(lisp_runtime *rt, lisp_scope *scope, lisp_list *a, void *user)
{
	lisp_list *args, *list;
	lisp_value *callable, *initializer;
	int length;
	(void) user; /* unused */

	args = lisp_eval_list(rt, scope, a);
	lisp_error_check(args);
	length = lisp_list_length(args);

	if (length == 2) {
		if (!lisp_get_args(rt, args, "*l", &callable, &list)) {
			return NULL;
		}
		if (lisp_list_length(list) < 2) {
			return lisp_error(rt, LE_2FEW, "reduce: list must have at least 2 entries");
		}
		initializer = list->left;
		list = (lisp_list*)list->right;
	} else if (length == 3) {
		if (!lisp_get_args(rt, args, "**l", &callable, &initializer, &list)) {
			return NULL;
		}
		if (lisp_list_length(list) < 1) {
			return lisp_error(rt, LE_2FEW, "reduce: list must have at least 1 entry");
		}
	} else {
		return lisp_error(rt, LE_2MANY, "reduce: 2 or 3 arguments required");
	}

	lisp_for_each(list) {
		initializer = lisp_call(rt, scope, callable,
		                        (lisp_value*) lisp_new_pair_list(rt, initializer, list->left));
		lisp_error_check(initializer);
	}
	return initializer;
}

static lisp_value *lisp_builtin_print(lisp_runtime *rt, lisp_scope *scope, lisp_list *a, void *user)
{
	lisp_value *v;
	lisp_list *args;
	(void) user; /* unused */

	args = lisp_eval_list(rt, scope, a);
	lisp_error_check(args);

	if (!lisp_get_args(rt, args, "*", &v)) {
		return NULL;
	}

	lisp_print(stdout, v);
	printf("\n");
	return lisp_nil_new(rt);
}

static lisp_value *lisp_builtin_dump_stack(lisp_runtime *rt, lisp_scope *scope,
                                           lisp_list *a, void *user)
{
	/* NB: This is very debugging oriented. We don't even eval arguments! */
	(void) scope; /* unused args */
	(void) a;
	(void) user;
	lisp_dump_stack(rt, NULL, stderr);
	return lisp_nil_new(rt);
}

static lisp_value *lisp_builtin_progn(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *a, void *user)
{
	(void) user; /* unused */
	return lisp_progn(rt, scope, a);
}

static lisp_value *lisp_builtin_unquote(lisp_runtime *rt, lisp_scope *scope,
                                        lisp_list *arglist, void *user)
{
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;
	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return lisp_eval(rt, scope, firstarg);
}

static lisp_value *lisp_quasiquote(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_value *v)
{
	lisp_list *vl;
	lisp_symbol *vlls;

	if (v->type != type_list || lisp_nil_p(v)) {
		return v;
	}

	vl = (lisp_list *) v;
	if (vl->left->type == type_symbol) {
		vlls = (lisp_symbol *) vl->left;
		if (strcmp(vlls->sym, "unquote") == 0) {
			return lisp_eval(rt, scope, v);
		}
	}
	lisp_for_each(vl) {
		vl->left = lisp_quasiquote(rt, scope, vl->left);
		if (!vl->left)
			return NULL;
	}
	return v;
}

static lisp_value *lisp_builtin_quasiquote(lisp_runtime *rt, lisp_scope *scope,
                                           lisp_list *arglist, void *user)
{
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;
	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return lisp_quasiquote(rt, scope, firstarg);
}

void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope)
{
	lisp_scope_add_builtin(rt, scope, "eval", lisp_builtin_eval, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "car", lisp_builtin_car, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "cdr", lisp_builtin_cdr, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "quote", lisp_builtin_quote, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "cons", lisp_builtin_cons, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "lambda", lisp_builtin_lambda, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "macro", lisp_builtin_macro, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "define", lisp_builtin_define, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "+", lisp_builtin_plus, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "-", lisp_builtin_minus, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "*", lisp_builtin_multiply, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "/", lisp_builtin_divide, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "==", lisp_builtin_eq, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "=", lisp_builtin_eq, NULL, 0);
	lisp_scope_add_builtin(rt, scope, ">", lisp_builtin_gt, NULL, 0);
	lisp_scope_add_builtin(rt, scope, ">=", lisp_builtin_ge, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "<", lisp_builtin_lt, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "<=", lisp_builtin_le, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "if", lisp_builtin_if, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "null?", lisp_builtin_null_p, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "map", lisp_builtin_map, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "reduce", lisp_builtin_reduce, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "print", lisp_builtin_print, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "dump-stack", lisp_builtin_dump_stack, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "progn", lisp_builtin_progn, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "unquote", lisp_builtin_unquote, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "quasiquote", lisp_builtin_quasiquote, NULL, 0);
}
