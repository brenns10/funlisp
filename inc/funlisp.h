/*
 * funlisp.h: public funlisp interface
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _FUNLISP_H
#define _FUNLISP_H

#include <stdio.h> /* for FILE* */

/**
 * This is a context object, which tracks all language objects which have been
 * created, and is used for garbage collection as well as holding any other
 * information about your instance of the interpreter. The context can be
 * created with lisp_runtime_new() and destroyed with lisp_runtime_free().
 */
typedef struct lisp_runtime lisp_runtime;

/**
 * Allocate and initialize a new runtime object. You must use
 * lisp_runtime_free() to cleanup every runtime you create.
 * @return new runtime
 */
lisp_runtime *lisp_runtime_new(void);

/**
 * Clean up all resources and free a runtime object.
 * @warning This will invoke the garbage collector, freeing every language
 * object associated with the runtime. Once calling this, ALL pointers to
 * funlisp objects become invalid.
 * @param rt runtime to free
 */
void lisp_runtime_free(lisp_runtime *rt);

/**
 * In funlisp, (almost) everything is a lisp_value -- that is, it can be cast to
 * a ``lisp_value *`` and operated on. Integers, Strings, Code, etc. The only
 * thing which is not a lisp_value is the lisp_runtime.
 */
typedef struct lisp_value {
	struct lisp_type  *type; /* type object, USE ME TO TYPE CHECK */
	struct lisp_value *next; /* don't touch me */
	char mark;               /* don't touch me */
} lisp_value;

/**
 * Type objects hold all the information & operations that a data type in
 * funlisp needs in order to be a part of the language. This includes the name
 * of the type, and operations such as printing, calling, evaluating, and some
 * garbage collection related functionality.
 */
typedef struct lisp_type lisp_type;

/**
 * Scope objects bind symbols to lisp_value's. In order for the language to
 * function correctly, the root scope needs to contain all of the language
 * built-in features. You can obtain a scope like this by calling
 * lisp_new_default_scope().
 */
typedef struct lisp_scope lisp_scope;

/**
 * Lisp is a list-processing language, and lisp_list is a building block for
 * lists. It is somewhat mis-named, because it actually represents a
 * s-expression, which is just a simple data structure that has two pointers:
 * left and right. Normal lists are a series of s-expressions, such that each
 * node contains a pointer to data in "left", and a pointer to the next node in
 * "right". S-expressions may be written in lisp like so:
 *
 *     > '(left . right)
 *     (left . right)
 *
 * Normal lists are simply syntactic sugar for a series of nested s-expressions:
 *
 *     > '(a . (b . '()))
 *     (a b )
 */
typedef struct lisp_list lisp_list;

/**
 * Symbols are tokens (non-numeric, non parentheses) which occur in funlisp
 * code, not surounded by double quotes. For example, in the following code:
 *
 *     (define abs
 *       (lambda (x)
 *         (if (< x 0)
 *           (- 0 x)
 *           x)))
 *
 *  The symbols are: define, abs, lambda, x, if, and <.
 */
typedef struct lisp_symbol lisp_symbol;

/**
 * Error is a lisp type returned whenever (shockingly) an error occurs. This is
 * a bit of a hack to enable a base support for error handling. Errors may have
 * a string message.
 */
typedef struct lisp_error lisp_error;

/**
 * lisp_integer contains an int object of whatever size the C implementation
 * supports.
 */
typedef struct lisp_integer lisp_integer;

/**
 * This is a string (which occurs quoted in lisp source)
 */
typedef struct lisp_string lisp_string;

/**
 * This data structure contains a native C function which may be called by
 * funlisp code. The C function must be of type lisp_builtin_func.
 */
typedef struct lisp_builtin lisp_builtin;

/**
 * Data structure implementing a lisp lambda function.
 */
typedef struct lisp_lambda lisp_lambda;

/**
 * Type object of lisp_type
 */
extern lisp_type *type_type;

/**
 * Type object of lisp_scope
 */
extern lisp_type *type_scope;

/**
 * Type object of lisp_list
 */
extern lisp_type *type_list;

/**
 * Type object of lisp_symbol
 */
extern lisp_type *type_symbol;

/**
 * Type object of lisp_error
 */
extern lisp_type *type_error;

/**
 * Type object of lisp_integer
 */
extern lisp_type *type_integer;

/**
 * Type object of lisp_string
 */
extern lisp_type *type_string;

/**
 * Type object of lisp_builtin
 */
extern lisp_type *type_builtin;

/**
 * Type object of lisp_lambda
 */
extern lisp_type *type_lambda;

/**
 * Prints a string representing ``value`` to ``f``. This output is not meant to
 * contain all the information necessary to recreate ``value``, just enough to
 * give you an idea what it is.
 * @param f file open for writing
 * @param value value to print
 */
void lisp_print(FILE *f, lisp_value *value);

/**
 * Evaluate the lisp_value in a given context. Since lisp values represent code
 * as well as data, this is more applicable to some data structures than others.
 * For example, evaluating a scope will not work. However, evaluating a symbol
 * will look it up in the current scope, and evaluating list ``l`` will attempt
 * to call ``(car l)`` with arguments ``(cdr l)``.
 * @param rt runtime associated with scope and value
 * @param scope the scope to use for evaluation (used when looking up symbols)
 * @param value the value (code generally) to evaluate
 * @return the result of evaluating value in scope
 */
lisp_value *lisp_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);

/**
 * Parse a *single* expression from a string, returning it as a lisp_value. If
 * there is no expression, return NULL
 * @param rt runtime to create language objects in
 * @param input string
 * @return parsed expression
 * @retval NULL on error or no expression available
 */
lisp_value *lisp_parse(lisp_runtime *rt, char *input);

/**
 * Call a callable object with a list of arguments. Many data types are not
 * callable, in which case a lisp_error is returned.
 * @param rt runtime
 * @param scope scope in which we are being evaluated
 * @param callable value to call
 * @param arguments a lisp_list containing arguments (which *have not yet been
 * evaluated*)
 */
lisp_value *lisp_call(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable,
                      lisp_value *arguments);

/**
 *
 * Parse an entire file of input, evaluating it within a scope as we go. Return
 * the result of evaluating the last expression in the file. This is typically
 * useful for loading a file before running main.  See lisp_run_main_if_exists()
 * @warning This function performs garbage collection as it evaluates each
 * expression, marking only the scope which it evaluates within.
 * @param rt runtime
 * @param scope scope to evaluate within (usually a default scope)
 * @param input file to load as funlisp code
 * @return the parsed code
 * @retval NULL on empty file, or file read error
 */
lisp_value *lisp_load_file(lisp_runtime *rt, lisp_scope *scope, FILE *input);

/**
 * Create a new scope containing the default builtins (lambda, define,
 * arithmetic operators, etc).
 * @param rt runtime
 * @returns new default scope
 */
lisp_scope *lisp_new_default_scope(lisp_runtime *rt);

/**
 * Create a new empty scope. This would be most useful when creating a new
 * nested scope, e.g. for a function body.
 * @param rt runtime
 * @returns new empty scope
 */
lisp_scope *lisp_new_empty_scope(lisp_runtime *rt);

/**
 * Mark an object as still reachable or useful to the program (or you). This can
 * be called several times to mark many objects. Marking objects prevents the
 * garbage collector from freeing them. The garbage collector performs a breadth
 * first search starting from your marked objects to find all reachable language
 * objects. Thus, marking an object like a lisp_scope will save all symbols and
 * language objects contained within it, from being freed. Normal use is to mark
 * and sweep each time you've evaluated something:
 *
 *     lisp_value *result = lisp_eval(rt, scope, some_cool_code);
 *     lisp_mark(rt, (lisp_value*) scope);
 *     lisp_mark(rt, result);
 *     lisp_sweep(rt);
 *
 * @warning Be explicit about marking. If we had left out the third line of the
 * code sample above, there's a good chance that ``result`` would have been
 * freed when ``lisp_sweep()`` was called.
 * @param rt runtime
 * @param v value to mark as still needed. This value, and all values reachable
 * from it, are preserved on the next ``lisp_sweep()`` call.
 */
void lisp_mark(lisp_runtime *rt, lisp_value *v);

/**
 * Free every object associated with the runtime, which is not marked or
 * reachable from a marked object.
 * @param rt runtime
 */
void lisp_sweep(lisp_runtime *rt);


/* UTILITIES */

/**
 * Convert the array of strings into a lisp list of string objects.
 * @param list an array of strings
 * @param n length of the array
 * @param can_free Does the interpreter take ownership of the memory pointed at
 * by the strings? If so, can_free should be non-zero. If not, it should be 0.
 * @return lisp_list containing lisp_string objects
 */
lisp_value *lisp_list_of_strings(lisp_runtime *rt, char **list, size_t n, char can_free);

/**
 * Given a lisp_value, put it inside a list of size 0 and return it.
 * @param rt runtime
 * @param entry item to put inside a list
 * @return a singleton list
 */
lisp_value *lisp_singleton_list(lisp_runtime *rt, lisp_value *entry);

/**
 * Lookup the symbol ``main`` in the scope, and run it if it exists. Calls the
 * function with a single argument, a lisp_list of program arguments. argc and
 * argv should not include the main executable (just the script name and args).
 * @param rt runtime
 * @param scope scope to find main in
 * @param argc number of arguments
 * @param argv NULL-terminated argument list
 * @returns result of evaluation
 * @retval NULL if no main function existed
 */
lisp_value *lisp_run_main_if_exists(lisp_runtime *rt, lisp_scope *scope,
                                    int argc, char **argv);

/**
 * A built-in function. Takes a runtime, scope of evaluation, and a list of
 * arguments.
 */
typedef lisp_value * (*lisp_builtin_func)(lisp_runtime*, lisp_scope*,lisp_value*);

/* Helper functions */
void lisp_scope_bind(lisp_scope *scope, lisp_symbol *symbol, lisp_value *value);
lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol);
lisp_value *lisp_scope_lookup_string(lisp_runtime *rt, lisp_scope *scope, char *name);
void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name, lisp_builtin_func call);
void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope);
lisp_value *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_value *list);
int lisp_get_args(lisp_list *list, char *format, ...);
lisp_value *lisp_quote(lisp_runtime *rt, lisp_value *value);
/* List functions */
int lisp_list_length(lisp_list *list);
int lisp_nil_p(lisp_value *l);

/* type creation and accessing */
lisp_string *lisp_string_new_unowned(lisp_runtime *rt, char *str);
lisp_string *lisp_string_new(lisp_runtime *rt, char *str);
char *lisp_string_get(lisp_string *s);

lisp_symbol *lisp_symbol_new(lisp_runtime *rt, char *string);
char *lisp_symbol_get(lisp_symbol *);

lisp_error  *lisp_error_new(lisp_runtime *rt, char *message);
char *lisp_error_get(lisp_error *);

lisp_list *lisp_list_new(lisp_runtime *rt, lisp_value *left, lisp_value *right);
lisp_value *lisp_list_get_left(lisp_list *);
lisp_value *lisp_list_get_right(lisp_list *);

lisp_integer *lisp_integer_new(lisp_runtime *rt, int n);
int lisp_integer_get(lisp_integer *integer);

lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call);
lisp_value *lisp_nil_new(lisp_runtime *rt);

#endif
