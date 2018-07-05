/*
 * builtins.c: the built-in functions of funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "funlisp_internal.h"

static lisp_value *lisp_builtin_eval(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *arguments, void *user)
{
	/* args are evaluated */
	(void) user; /* unused */
	return lisp_eval(rt, scope, arguments->left);
}

static lisp_value *lisp_builtin_car(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_list *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "l", &firstarg)) {
		return NULL;
	}
	if (lisp_nil_p((lisp_value*) firstarg)) {
		return lisp_error(rt, LE_VALUE, "car of nil list");
	}
	return firstarg->left;
}

static lisp_value *lisp_builtin_cdr(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_list *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "l", &firstarg)) {
		return NULL;
	}
	if (lisp_nil_p((lisp_value*) firstarg)) {
		return lisp_error(rt, LE_VALUE, "cdr of nil list");
	}
	return firstarg->right;
}

static lisp_value *lisp_builtin_quote(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *arglist, void *user)
{
	/* args NOT evaluated */
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return arglist->left;
}

static lisp_value *lisp_builtin_cons(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_value *a1, *l;
	lisp_list *new;
	(void) user; /* unused */
	(void) scope;

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
	/* args NOT evaluated */
	lisp_list *argnames, *code, *it;
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
	/* args NOT evaluated */
	lisp_list *argnames, *code, *it;
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
	/* args NOT evaluated */
	lisp_symbol *s;
	lisp_value *expr;
	(void) user; /* unused */

	if (!lisp_get_args(rt, a, "s*", &s, &expr)) {
		return NULL;
	}

	expr = lisp_eval(rt, scope, expr);
	lisp_error_check(expr);
	lisp_scope_bind(scope, s, expr);
	return expr;
}

static lisp_value *lisp_builtin_plus(lisp_runtime *rt, lisp_scope *scope,
                                     lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_integer *i;
	int sum = 0;
	(void) user; /* unused */
	(void) scope;

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
                                      lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_integer *i;
	int val = 0, len;
	(void) user; /* unused */
	(void) scope;

	len = lisp_list_length(args);

	if (len < 1) {
		return lisp_error(rt, LE_2FEW, "expected at least one arg");
	} else if (len == 1) {
		i = (lisp_integer*) args->left;
		val = - i->x;
	} else {
		if (args->left->type != type_integer) {
			return lisp_error(rt, LE_TYPE, "expected integer");
		}
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
                                         lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_integer *i;
	int product = 1;
	(void) user; /* unused */
	(void) scope;

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
                                       lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_integer *i;
	int val = 0, len;
	(void) user; /* unused */
	(void) scope;

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
			return lisp_error(rt, LE_VALUE, "divide by zero");
		}
		val /= i->x;
	}

	i = (lisp_integer*)lisp_new(rt, type_integer);
	i->x = val;
	return (lisp_value*)i;
}

#define CMP_EQ (void*) 1
#define CMP_NE (void*) 2
#define CMP_LT (void*) 3
#define CMP_LE (void*) 4
#define CMP_GT (void*) 5
#define CMP_GE (void*) 6

static lisp_value *lisp_builtin_cmp(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *args, void *op)
{
	/* args are evaluated */
	lisp_integer *first, *second, *result;
	(void) scope; /* unused */

	if (!lisp_get_args(rt, args, "dd", &first, &second)) {
		return NULL;
	}

	result = (lisp_integer*)lisp_new(rt, type_integer);
	if (op == CMP_EQ) {
		result->x = first->x == second->x;
	} else if (op == CMP_NE) {
		result->x = first->x != second->x;
	} else if (op == CMP_LT) {
		result->x = first->x < second->x;
	} else if (op == CMP_LE) {
		result->x = first->x <= second->x;
	} else if (op == CMP_GT) {
		result->x = first->x > second->x;
	} else {
		result->x = first->x >= second->x;
	}

	return (lisp_value*)result;
}

static lisp_value *lisp_builtin_if(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *a, void *user)
{
	/* args NOT evaluated */
	lisp_value *condition, *body_true, *body_false;
	(void) user; /* unused */

	if (!lisp_get_args(rt, a, "***", &condition, &body_true, &body_false)) {
		return NULL;
	}

	condition = lisp_eval(rt, scope, condition);
	lisp_error_check(condition);
	if (lisp_truthy(condition)) {
		return lisp_eval(rt, scope, body_true);
	} else {
		return lisp_eval(rt, scope, body_false);
	}
}

static lisp_value *lisp_builtin_null_p(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_integer *result;
	lisp_value *v;
	(void) user; /* unused */
	(void) scope;

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
		/* Termination condition: one of the lists of args ends */
		if (lisp_nil_p(list_of_lists->left)) {
			return NULL;
		}
		/* Get the next node in the list and get the argument. */
		l = (lisp_list*) list_of_lists->left;
		left_items->left = (lisp_value*) lisp_quote(rt, l->left);
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

		/* Get the next node in the list and get the argument. */
		l = (lisp_list*) list_of_lists->left;
		right_items->left = l->right;
	}
	right_items->right = lisp_nil_new(rt);
	return rv;
}

static lisp_value *lisp_builtin_map(lisp_runtime *rt, lisp_scope *scope,
                                    lisp_list *map_args, void *user)
{
	/* args are evaluated */
	lisp_value *f;
	lisp_list *ret = NULL, *rv, *args;
	(void) user; /* unused */

	/* Get the function from the first argument in the list. */
	f = map_args->left;
	if (lisp_nil_p(map_args->right)) {
		return lisp_error(rt, LE_2FEW, "need at least two arguments");
	}
	map_args = (lisp_list*) map_args->right;

	/* Make sure the arguments are well-behaved lists */
	if (lisp_is_bad_list_of_lists(map_args)) {
		return lisp_error(rt, LE_VALUE,
			"arguments after callable must be lists");
	}
	while ((args = get_quoted_left_items(rt, map_args)) != NULL) {
		if (ret == NULL) {
			ret = (lisp_list*) lisp_new(rt, type_list);
			rv = ret;
		} else {
			ret->right = lisp_new(rt, type_list);
			ret = (lisp_list*) ret->right;
		}
		ret->left = lisp_call(rt, scope, f, args);
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

static lisp_value *lisp_builtin_reduce(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *args, void *user)
{
	/* args are evaluated */
	lisp_list *list;
	lisp_value *callable, *initializer;
	int length;
	(void) user; /* unused */

	length = lisp_list_length(args);

	if (length == 2) {
		if (!lisp_get_args(rt, args, "*l", &callable, &list)) {
			return NULL;
		}
		if (lisp_list_length(list) < 2) {
			return lisp_error(rt, LE_VALUE, "reduce: list must have at least 2 entries");
		}
		initializer = list->left;
		list = (lisp_list*)list->right;
	} else if (length == 3) {
		if (!lisp_get_args(rt, args, "**l", &callable, &initializer, &list)) {
			return NULL;
		}
		if (lisp_list_length(list) < 1) {
			return lisp_error(rt, LE_VALUE, "reduce: list must have at least 1 entry");
		}
	} else if (length <= 2) {
		return lisp_error(rt, LE_2FEW, "reduce: 2 or 3 arguments required");
	} else {
		return lisp_error(rt, LE_2MANY, "reduce: 2 or 3 arguments required");
	}

	lisp_for_each(list) {
		initializer = lisp_call(rt, scope, callable,
			lisp_new_pair_list(rt, initializer, list->left));
		lisp_error_check(initializer);
	}
	return initializer;
}

static lisp_value *lisp_builtin_print(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *args, void *user)
{
	/* args are evaluated */
	(void) user; /* unused */
	(void) scope;

	lisp_for_each(args) {
		lisp_print(stdout, args->left);
	}

	printf("\n");
	return lisp_nil_new(rt);
}

static lisp_value *lisp_builtin_dump_stack(lisp_runtime *rt, lisp_scope *scope,
                                           lisp_list *a, void *user)
{
	/* args are evaluated (but unused) */
	(void) scope; /* unused args */
	(void) a;
	(void) user;
	lisp_dump_stack(rt, NULL, stderr);
	return lisp_nil_new(rt);
}

static lisp_value *lisp_builtin_progn(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *a, void *user)
{
	/* args NOT evaluated */
	(void) user; /* unused */
	return lisp_progn(rt, scope, a);
}

static lisp_value *lisp_builtin_unquote(lisp_runtime *rt, lisp_scope *scope,
                                        lisp_list *arglist, void *user)
{
	/* args NOT evaluated */
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return lisp_eval(rt, scope, firstarg);
}

static lisp_value *lisp_quasiquote(lisp_runtime *rt, lisp_scope *scope,
                                   void *user, lisp_value *v)
{
	lisp_list *vl;
	lisp_symbol *vlls;
	(void)user;

	if (v->type != type_list || lisp_nil_p(v)) {
		return v;
	}

	vl = (lisp_list *) v;
	if (vl->left->type == type_symbol) {
		vlls = (lisp_symbol *) vl->left;
		if (strcmp(vlls->s, "unquote") == 0) {
			return lisp_eval(rt, scope, v);
		}
	}

	return (lisp_value*) lisp_map(rt, scope, NULL, lisp_quasiquote, vl);
}

static lisp_value *lisp_builtin_quasiquote(lisp_runtime *rt, lisp_scope *scope,
                                           lisp_list *arglist, void *user)
{
	/* args NOT evaluated */
	lisp_value *firstarg;
	(void) user; /* unused */
	(void) scope;

	if (!lisp_get_args(rt, arglist, "*", &firstarg)) {
		return NULL;
	}
	return lisp_quasiquote(rt, scope, NULL, firstarg);
}

static lisp_value *lisp_builtin_eq(lisp_runtime *rt, lisp_scope *scope,
                                   lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_value *lhs, *rhs;
	(void) user;
	(void) scope;

	if (!lisp_get_args(rt, arglist, "**", &lhs, &rhs)) {
		return NULL;
	}

	return (lisp_value*) lisp_integer_new(rt, lhs == rhs);
}

static lisp_value *lisp_builtin_equal(lisp_runtime *rt, lisp_scope *scope,
                                      lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_value *lhs, *rhs;
	(void) user;
	(void) scope;

	if (!lisp_get_args(rt, arglist, "**", &lhs, &rhs)) {
		return NULL;
	}

	return (lisp_value*) lisp_integer_new(rt, lisp_compare(lhs, rhs));
}

static lisp_value *lisp_builtin_assert(lisp_runtime *rt, lisp_scope *scope,
                                       lisp_list *arglist, void *user)
{
	/* args are evaluated */
	lisp_integer *expr;
	(void) user;
	(void) scope;

	if (!lisp_get_args(rt, arglist, "d", &expr))
		return NULL;

	if (expr->x == 0)
		return lisp_error(rt, LE_ASSERT, "assertion error");
	else
		return (lisp_value*) expr;
}

static lisp_value *lisp_builtin_assert_error(
		lisp_runtime *rt, lisp_scope *scope, lisp_list *arglist,
		void *user)
{
	/* args are NOT evaluated, to avoid error handling short circuit */
	lisp_value *sym, *expr;
	lisp_symbol *sym_evald;
	enum lisp_errno errno;
	(void) user;

	if (!lisp_get_args(rt, arglist, "**", &sym, &expr))
		return NULL;

	sym_evald = (lisp_symbol*) lisp_eval(rt, scope, sym);
	lisp_error_check(sym_evald);
	if (sym_evald->type != type_symbol)
		return lisp_error(rt, LE_TYPE, "error type must be symbol");
	errno = lisp_sym_to_errno(sym_evald);
	if (errno == LE_MAX_ERR)
		return lisp_error(rt, LE_VALUE, "unrecognized error type");

	lisp_eval(rt, scope, expr); /* we don't care, cause we expect error */
	/* NO ERROR CHECK HERE */

	if (errno == lisp_get_errno(rt)) {
		lisp_clear_error(rt);
		return (lisp_value*) sym_evald;
	} else {
		fprintf(stderr, "Assertion error! Expected %s\n",
			lisp_error_name[errno]);
		fprintf(stderr, "This was the actual error encountered: ");
		lisp_print_error(rt, stderr);
		fprintf(stderr, "\nBelow should be the assertion error stack trace.\n");
		lisp_clear_error(rt);
		return lisp_error(rt, LE_ASSERT, "assertion error");
	}
}

/*
 * (cond
 *    (TEST1 VALUE2)
 *   [(TEST2 VALUE2) ...]
 * )
 */
static lisp_value *lisp_builtin_cond(
		lisp_runtime *rt, lisp_scope *scope, lisp_list *arglist, void *user)
{
	/* args NOT evaluated */
	lisp_list *clause, *node;
	lisp_value *expr, *value;
	(void) user; /* unused */

	if (lisp_nil_p((lisp_value*)arglist))
		return lisp_error(rt, LE_SYNTAX, "bad syntax for cond");

	lisp_for_each(arglist) {
		if (arglist->left->type != type_list)
			return lisp_error(rt, LE_SYNTAX, "bad syntax for cond");
		clause = (lisp_list*) arglist->left;

		if (lisp_is_bad_list(clause) || lisp_list_length(clause) != 2)
			return lisp_error(rt, LE_SYNTAX, "bad syntax for cond");

		expr = clause->left;
		node = (lisp_list*) clause->right;
		value = node->left;

		expr = lisp_eval(rt, scope, expr);
		lisp_error_check(expr);

		if (lisp_truthy(expr))
			return lisp_eval(rt, scope, value);
	}
	return lisp_nil_new(rt);
}

void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope)
{
	lisp_scope_add_builtin(rt, scope, "eval", lisp_builtin_eval, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "car", lisp_builtin_car, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "cdr", lisp_builtin_cdr, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "quote", lisp_builtin_quote, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "cons", lisp_builtin_cons, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "lambda", lisp_builtin_lambda, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "macro", lisp_builtin_macro, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "define", lisp_builtin_define, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "+", lisp_builtin_plus, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "-", lisp_builtin_minus, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "*", lisp_builtin_multiply, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "/", lisp_builtin_divide, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "==", lisp_builtin_cmp, CMP_EQ, 1);
	lisp_scope_add_builtin(rt, scope, "=", lisp_builtin_cmp, CMP_EQ, 1);
	lisp_scope_add_builtin(rt, scope, "!=", lisp_builtin_cmp, CMP_NE, 1);
	lisp_scope_add_builtin(rt, scope, ">", lisp_builtin_cmp, CMP_GT, 1);
	lisp_scope_add_builtin(rt, scope, ">=", lisp_builtin_cmp, CMP_GE, 1);
	lisp_scope_add_builtin(rt, scope, "<", lisp_builtin_cmp, CMP_LT, 1);
	lisp_scope_add_builtin(rt, scope, "<=", lisp_builtin_cmp, CMP_LE, 1);
	lisp_scope_add_builtin(rt, scope, "if", lisp_builtin_if, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "null?", lisp_builtin_null_p, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "map", lisp_builtin_map, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "reduce", lisp_builtin_reduce, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "print", lisp_builtin_print, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "dump-stack", lisp_builtin_dump_stack, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "progn", lisp_builtin_progn, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "unquote", lisp_builtin_unquote, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "quasiquote", lisp_builtin_quasiquote, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "eq?", lisp_builtin_eq, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "equal?", lisp_builtin_equal, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "assert", lisp_builtin_assert, NULL, 1);
	lisp_scope_add_builtin(rt, scope, "assert-error", lisp_builtin_assert_error, NULL, 0);
	lisp_scope_add_builtin(rt, scope, "cond", lisp_builtin_cond, NULL, 0);
}
