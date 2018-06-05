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
 * In funlisp, (almost) everything is a lisp_value. That is, it can be cast to
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
 * @param rt runtime
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

/**
 * Bind a symbol to a value in a scope.
 * @param scope scope to define the name in
 * @param symbol symbol that is the name
 * @param value what the symbol is bound to
 */
void lisp_scope_bind(lisp_scope *scope, lisp_symbol *symbol, lisp_value *value);

/**
 * Look up a symbol within a scope. If it is not found in this scope, look
 * within the parent scope etc, until it is found. If it is not found at all,
 * return a lisp_error object.
 * @param rt runtime
 * @param scope scope to look in
 * @param symbol symbol to look up
 * @return value found, or a lisp_error when not found
 */
lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol);
/**
 * Lookup a name within a scope, without creating a symbol object. Behaves the
 * same as lisp_scope_lookup().
 * @param rt runtime
 * @param scope scope to look in
 * @param name string name to look up
 * @return value found, or a lisp_error when not found
 */
lisp_value *lisp_scope_lookup_string(lisp_runtime *rt, lisp_scope *scope, char *name);

/**
 * Shortcut to declare a builtin function. Simply takes a function pointer and a
 * string name, and it will internally create the lisp_builtin object with the
 * correct name, and bind it in the given scope.
 * @param rt runtime
 * @param scope scope to bind builtin in
 * @param name name of builtin
 * @param call function pointer defining the builtin
 */
void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name, lisp_builtin_func call);

/**
 * Add all language defaults to a scope. This is critical for the language work,
 * at all, since most language elements are implemented as builtin functions.
 * @param rt runtime
 * @param scope scope to add builtins too
 */
void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope);

/**
 * Given a list of arguments, evaluate each of them within a scope and return a
 * new list containing the evaluated arguments. This is most useful for
 * implementing builtin functions.
 * @param rt runtime
 * @param scope scope to evaluate within
 * @param list list of un-evaluated function arguments
 * @return list of evaluated function arguments
 */
lisp_value *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_value *list);

/**
 * Given a list of function arguments, perform type checking and verify the
 * number of arguments according to a format string. The following formats are
 * recognized:
 *
 *     d - integer
 *     l - list
 *     s - symbol
 *     S - string
 *     o - scope
 *     e - error
 *     b - builtin
 *     t - type
 *     * - anything
 *
 * As an example, a function which takes an integer and a string, and prints the
 * string N times, might use the format string ``dS``.
 *
 * The remaining variadic arguments are pointers to object pointers, and they
 * will be assigned as each argument is parsed. EG:
 *
 *     lisp_integer *arg1;
 *     lisp_string *arg2;
 *     lisp_get_args(args, "dS", &arg1, &arg2);
 *
 * @param list Argument list to type check and count
 * @param format Format string
 * @param ... Destination pointer to place results
 * @retval 1 on success (true)
 * @retval 0 on failure (false)
 */
int lisp_get_args(lisp_list *list, char *format, ...);

/**
 * Return value, but inside a list containing the symbol ``quote``. When this
 * evaluated, it will return its contents (``value``) un-evaluated.
 * @param rt runtime
 * @param value value to return quoted
 * @return value but quoted
 */
lisp_value *lisp_quote(lisp_runtime *rt, lisp_value *value);

/**
 * Return the length of a list.
 * @param list list to find the length of
 * @return length of the list
 */
int lisp_list_length(lisp_list *list);

/**
 * Return true if the lisp_value is "nil" (an empty list).
 * @param l value to check
 * @retval 1 (true) if l is nil
 * @retval 0 (false) if l is non-nil
 */
int lisp_nil_p(lisp_value *l);

/**
 * Return a new, "un-owned" string. "Un-owned" means that ``str`` will not be
 * freed when the lisp_string is garbage collected. However, the lisp_string
 * will still contain the exact reference to ``str``, not a copy. So, your
 * application **must not** free ``str`` until the lisp_string containing it is
 * garbage collected. The safest approach here is if you are certain that your
 * string will not be freed until after the lisp_runtime is freed.
 *
 * @code
 *     lisp_value *v = (lisp_string *) lisp_string_new_unowned(rt, "hello");
 * @endcode
 *
 * @note This is the ideal function to use with string literals, since they are
 * statically allocated.
 * @param rt runtime
 * @param str string which will not be freed by the garbage collector
 * @return lisp_string object pointing to your string
 */
lisp_string *lisp_string_new_unowned(lisp_runtime *rt, char *str);

/**
 * Return a new string. This function takes a "safe" approach, by copying your
 * string and using the copy. The pointer will be owned by the interpreter and
 * freed when the lisp_string object is garbage collected. This is roughly
 * equivalent to duplicating the string using strdup(), and then creating a new
 * owned string with that pointer.
 * @note This is also safe to use with string literals, but it is not the most
 * efficient way, since the string gets copied.
 * @param rt runtime
 * @param str string to copy and use in an owned string
 * @return a new lisp_string
 */
lisp_string *lisp_string_new(lisp_runtime *rt, char *str);

/**
 * Return a pointer to the string contained within a lisp_string. The
 * application must **not** modify or free the string.
 * @param s the lisp string to access
 * @return the contained string
 */
char *lisp_string_get(lisp_string *s);

/**
 * Return a new symbol. This function will copy the ``string`` and free the copy
 * it on garbage collection (much like lisp_string_new()).
 * @param rt runtime
 * @param string the symbol to create
 * @return the resulting symbol
 */
lisp_symbol *lisp_symbol_new(lisp_runtime *rt, char *string);

/**
 * Return the string contained in the symbol.
 * @param s the symbol to retrieve the string from
 * @return the string contained in the symbol
 */
char *lisp_symbol_get(lisp_symbol *s);

/**
 * Return a new error. This function will copy the ``message`` and free the copy
 * on garbage collection (much like lisp_string_new()).
 * @param rt runtime
 * @param message message to use for creating the error
 * @return a new error
 */
lisp_error  *lisp_error_new(lisp_runtime *rt, char *message);

/**
 * Return the message from an error.
 * @param e the error to retrieve the message from
 * @return the message contained in the error
 */
char *lisp_error_get(lisp_error *e);

/**
 * Create a new list node with left and right value already specified. This
 * interface only allows you to create lists from end to beginning.
 * @param rt runtime
 * @param left item to go on the left side of the s-expression, usually a list
 * item
 * @param right item to go on the right side of the s-expression, usually the
 * next lisp_list instance
 * @return newly allocated lisp_list
 */
lisp_list *lisp_list_new(lisp_runtime *rt, lisp_value *left, lisp_value *right);

/**
 * Retrieve the left item of a list node / sexp.
 * @param l list to retrieve from
 * @return left item of list node
 */
lisp_value *lisp_list_get_left(lisp_list *l);

/**
 * Retrieve the right item of a list node / sexp
 * @param l list to retrieve from
 * @return right item of list node
 */
lisp_value *lisp_list_get_right(lisp_list *l);

/**
 * Create a new integer.
 * @param rt runtime
 * @param n the integer value
 * @return newly allocated integer
 */
lisp_integer *lisp_integer_new(lisp_runtime *rt, int n);

/**
 * Retrieve the integer value from a lisp_integer.
 * @param integer lisp_integer to return from
 * @return the int value
 */
int lisp_integer_get(lisp_integer *integer);

/**
 * Create a new lisp_builtin from a function pointer, with a given name.
 * @warning Builtin names are not garbage collected, since they are almost
 * always static. If you need your name to be dynamically allocated, you'll have
 * to free it after you free the runtime.
 * @param rt runtime
 * @param name name of the builtin. the interpreter will never free the name!
 * @param call function pointer of the builtin
 * @return new builtin object
 */
lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call);

/**
 * Return a nil instance. Nil is simply a "special" lisp_list, with left and
 * right both set to NULL. It is used to terminate lists. For example, the list
 * ``'(a b)`` is internally: ``lisp_list(a, lisp_list(b, lisp_list(NULL, NULL)))``
 * @note This function is named "new" for uniformity. However, it does't
 * actually allocate a "new" nil value every time. Instead, each lisp_runtime
 * has a singleton nil instance, which is never garbage collected.
 * @param rt runtime
 * @return the nil value
 */
lisp_value *lisp_nil_new(lisp_runtime *rt);

#endif
