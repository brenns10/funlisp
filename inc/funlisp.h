/*
 * funlisp.h: public funlisp interface
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _FUNLISP_H
#define _FUNLISP_H

#include <stdio.h> /* for FILE* */

/**
 * @defgroup runtime Funlisp Runtime
 * @{
 */

/**
 * This is a context object, which tracks all language objects which have been
 * created, and is used for garbage collection as well as holding any other
 * information about your instance of the interpreter. The context can be
 * created with lisp_runtime_new() and destroyed with lisp_runtime_free().
 * The context is passed to nearly every function in the library, and builtin
 * functions receive it as well.
 *
 * The context may contain a "user context" (simply a void pointer) that an
 * embedding application may want its builtin functions to have access to.
 * Context is added with lisp_runtime_set_ctx() and retrieved with
 * lisp_runtime_get_ctx().
 */
typedef struct lisp_runtime lisp_runtime;

/**
 * Allocate and initialize a new runtime object. You must use
 * lisp_runtime_free() to cleanup every runtime you create.
 * @return new runtime
 */
lisp_runtime *lisp_runtime_new(void);

/**
 * Set the user context of a ::lisp_runtime.
 * @param rt runtime
 * @param user user context to set
 */
void lisp_runtime_set_ctx(lisp_runtime *rt, void *user);

/**
 * Get the user context from a ::lisp_runtime.
 * @param rt runtime
 * @return the user context object
 */
void *lisp_runtime_get_ctx(lisp_runtime *rt);

/**
 * Clean up all resources and free a runtime object.
 * @warning This will invoke the garbage collector, freeing every language
 * object associated with the runtime. Once calling this, ALL pointers to
 * funlisp objects become invalid.
 * @param rt runtime to free
 */
void lisp_runtime_free(lisp_runtime *rt);

/** @} */

/*
 * Although we use Doxygen @defgroup to group items into modules, we can't put
 * all declarations in their module group, because we would get compiler errors
 * about unknown types.  So in this section, we put all our structure
 * declarations and documentation. We use @ingroup to add them to their
 * respective modules. Then, we resume using modules below.
 */

/**
 * In funlisp, (almost) everything is a ::lisp_value. That is, it can be cast to
 * a ``lisp_value *`` and operated on. Integers, Strings, Code, etc. The only
 * thing which is not a ::lisp_value is the ::lisp_runtime.
 * @ingroup value
 */
typedef struct lisp_value lisp_value;

/**
 * A type object is a ::lisp_value containing operations that must be supported
 * by every type of object. It is not garbage collected, and every ::lisp_value
 * contains a pointer to its type object (even lisp_types themselves!).
 *
 * The only external use for a type object is that you can use it wih lisp_is()
 * to type check any ::lisp_value. Every type named lisp_X will have a
 * corresponding type_X object available.
 *
 * @sa lisp_is()
 * @ingroup value
 */
typedef struct lisp_type lisp_type;

/**
 * Scope objects bind ::lisp_symbol's to ::lisp_value's. In order for the
 * language to function correctly, the root scope needs to contain all of the
 * language built-in features. You can obtain a scope like this by calling
 * lisp_new_default_scope(), or you can create an empty one with
 * lisp_new_empty_scope().
 * @ingroup scope
 */
typedef struct lisp_scope lisp_scope;

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
 * The symbols are: define, abs, lambda, x, if, and <.
 * @ingroup types
 */
typedef struct lisp_symbol lisp_symbol;

/**
 * ::lisp_integer contains an int object of whatever size the C implementation
 * supports.
 * @ingroup types
 */
typedef struct lisp_integer lisp_integer;

/**
 * This is a string (which occurs quoted in lisp source)
 * @ingroup types
 */
typedef struct lisp_string lisp_string;

/**
 * This data structure contains a native C function which may be called by
 * funlisp code. The C function must be of type ::lisp_builtin_func.
 * @ingroup types
 */
typedef struct lisp_builtin lisp_builtin;

/**
 * Data structure implementing a lisp lambda function.
 * @ingroup types
 */
typedef struct lisp_lambda lisp_lambda;

/**
 * @defgroup value Lisp Values
 * @{
 */

/**
 * Type object of ::lisp_type, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_type;

/**
 * Prints a string representing @a value to @a f. This output is not meant to
 * contain all the information necessary to recreate @a value, just enough to
 * give you an idea what it is.
 * @param f file open for writing
 * @param value value to print
 */
void lisp_print(FILE *f, lisp_value *value);

/**
 * Evaluate the ::lisp_value in a given context. Since lisp values represent
 * code as well as data, this is more applicable to some data structures than
 * others.  For example, evaluating a scope will not work. However, evaluating a
 * symbol will look it up in the current scope, and evaluating list ``l`` will
 * attempt to call ``(car l)`` with arguments ``(cdr l)``.
 *
 * When an error occurs during execution, this function returns NULL and sets
 * the internal error details within the runtime.
 *
 * @param rt runtime associated with scope and value
 * @param scope the scope to use for evaluation (used when looking up symbols)
 * @param value the value to evaluate
 * @return the result of evaluating @a value in @a scope
 * @retval NULL when an error occurs
 */
lisp_value *lisp_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);

/**
 * Call a callable object with a list of arguments. Many data types are not
 * callable, in which case a NULL is returned and an error is set within the
 * runtime.
 * @param rt runtime
 * @param scope scope in which we are being evaluated
 * @param callable value to call
 * @param arguments a ::lisp_list containing arguments (which *have not yet been
 * evaluated*)
 * @return the result of calling @a callable with args @a arguments in scope @a
 * scope.
 * @retval NULL when an error occurs
 */
lisp_value *lisp_call(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable,
                      lisp_value *arguments);

/**
 * Perform type checking. Returns true (non-zero) when @a value has type @a
 * type.
 * @code
 * lisp_value *v = lisp_eval(rt, some_code, some_scope);
 * if (lisp_is(v, type_list)) {
 *     // do something based on this
 * }
 * @endcode
 * @param value value to type-check
 * @param type type object for the type you're interested in
 * @retval true (non-zero) if @a value has type @a type
 * @retval false (zero) if @a value is not of type @a type
 */
int lisp_is(lisp_value *value, lisp_type *type);

/**
 * @}
 * @defgroup scope Lisp Scopes
 * @{
 */

/**
 * Type object of ::lisp_scope, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_scope;

/**
 * Create a new scope containing the default builtins (lambda, define,
 * arithmetic operators, etc). This is just a shortcut for using
 * lisp_new_empty_scope() followed by lisp_scope_populate_builtin().
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
 * Add all language defaults to a scope. This is critical for the language work,
 * at all, since most language elements are implemented as builtin functions.
 * This function is used internally by lisp_new_default_scope().
 * @param rt runtime
 * @param scope scope to add builtins too
 */
void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope);

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
 * return NULL and set an error within the interpreter.
 * @param rt runtime
 * @param scope scope to look in
 * @param symbol symbol to look up
 * @return value found, or a NULL when not found
 */
lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol);
/**
 * Lookup a name within a scope. Uses a string argument rather than a
 * ::lisp_symbol object. Behavior is the same as lisp_scope_lookup().
 * @param rt runtime
 * @param scope scope to look in
 * @param name string name to look up
 * @return value found, or NULL when not found
 */
lisp_value *lisp_scope_lookup_string(lisp_runtime *rt, lisp_scope *scope, char *name);

/**
 * @}
 * @defgroup list Lisp Lists
 * @{
 */

/**
 * Lisp is a list-processing language, and ::lisp_list is a building block for
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
 * Type object of ::lisp_list, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_list;

/**
 * Create a new list node with left and right value already specified. This
 * interface only allows you to create lists from end to beginning.
 * @param rt runtime
 * @param left item to go on the left side of the s-expression, usually a list
 * item
 * @param right item to go on the right side of the s-expression, usually the
 * next ::lisp_list instance
 * @return newly allocated ::lisp_list
 */
lisp_list *lisp_list_new(lisp_runtime *rt, lisp_value *left, lisp_value *right);

/**
 * Given a ::lisp_value, put it inside a list of size 0 and return it.
 * @param rt runtime
 * @param entry item to put inside a list
 * @return a singleton list
 */
lisp_value *lisp_singleton_list(lisp_runtime *rt, lisp_value *entry);

/**
 * Convert the array of strings into a lisp list of string objects.
 * @param rt runtime
 * @param list an array of strings
 * @param n length of the array
 * @param can_free Does the interpreter take ownership of the memory pointed at
 * by the strings? If so, can_free should be non-zero. If not, it should be 0.
 * @return ::lisp_list containing ::lisp_string objects
 */
lisp_value *lisp_list_of_strings(lisp_runtime *rt, char **list, size_t n, char can_free);

/**
 * Return the length of a list.
 * @param list list to find the length of
 * @return length of the list
 */
int lisp_list_length(lisp_list *list);

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
 * Return a nil instance. Nil is simply a "special" ::lisp_list, with left and
 * right both set to NULL. It is used to terminate lists. For example, the list
 * ``'(a b)`` is internally: ``lisp_list(a, lisp_list(b, lisp_list(NULL, NULL)))``
 * @note This function is named "new" for uniformity. However, it does't
 * actually allocate a "new" nil value every time. Instead, each ::lisp_runtime
 * has a singleton nil instance, which is never garbage collected.
 * @param rt runtime
 * @return the nil value
 */
lisp_value *lisp_nil_new(lisp_runtime *rt);

/**
 * Return true if the ::lisp_value is "nil" (an empty list).
 * @param l value to check
 * @retval 1 (true) if l is nil
 * @retval 0 (false) if l is non-nil
 */
int lisp_nil_p(lisp_value *l);

/**
 * @}
 * @defgroup types Lisp Types
 * @{
 */

/**
 * Type object of ::lisp_symbol, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_symbol;

/**
 * Type object of ::lisp_integer, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_integer;

/**
 * Type object of ::lisp_string, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_string;

/**
 * Type object of ::lisp_builtin, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_builtin;

/**
 * Type object of ::lisp_lambda, for type checking.
 * @sa lisp_is()
 */
extern lisp_type *type_lambda;

/**
 * Return a new, "un-owned" string. "Un-owned" means that @a str will not be
 * freed when the ::lisp_string is garbage collected. However, the ::lisp_string
 * will still contain the exact reference to @a str, not a copy. So, your
 * application **must not** free @a str until the ::lisp_string containing it is
 * garbage collected. The safest approach here is if you are certain that your
 * string will not be freed until after the ::lisp_runtime is freed.
 *
 * @code
 *     lisp_value *v = (lisp_string *) lisp_string_new_unowned(rt, "hello");
 * @endcode
 *
 * @note This is the ideal function to use with string literals, since they are
 * statically allocated.
 * @param rt runtime
 * @param str string which will not be freed by the garbage collector
 * @return ::lisp_string object pointing to your string
 */
lisp_string *lisp_string_new_unowned(lisp_runtime *rt, char *str);

/**
 * Return a new string. This function takes a "safe" approach, by copying your
 * string and using the copy. The pointer will be owned by the interpreter and
 * freed when the ::lisp_string object is garbage collected. This is roughly
 * equivalent to duplicating the string using strdup(), and then creating a new
 * owned string with that pointer.
 * @note This is also safe to use with string literals, but it is not the most
 * efficient way, since the string gets copied.
 * @param rt runtime
 * @param str string to copy and use in an owned string
 * @return a new ::lisp_string
 */
lisp_string *lisp_string_new(lisp_runtime *rt, char *str);

/**
 * Return a pointer to the string contained within a ::lisp_string. The
 * application must **not** modify or free the string.
 * @param s the lisp string to access
 * @return the contained string
 */
char *lisp_string_get(lisp_string *s);

/**
 * Return a new symbol. This function will copy the @a string and free the copy
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
 * Create a new integer.
 * @param rt runtime
 * @param n the integer value
 * @return newly allocated integer
 */
lisp_integer *lisp_integer_new(lisp_runtime *rt, int n);

/**
 * Retrieve the integer value from a ::lisp_integer.
 * @param integer ::lisp_integer to return from
 * @return the int value
 */
int lisp_integer_get(lisp_integer *integer);

/**
 * @}
 * @defgroup builtins Builtin Functions
 * @{
 */

/**
 * A built-in function. Takes four arguments:
 * 1. The ::lisp_runtime associated with it. This may be used to retrieved the
 *    runtime's user context object (see lisp_runtime_get_ctx()).
 * 2. The ::lisp_scope this function is being called executed within. Most
 *    builtin functions will want to evaluate this with lisp_eval_list().
 * 3. The arguments to this function, as a ::lisp_list. These have not yet been
 *    evaluated.
 * 4. The user context associated with this builtin.
 */
typedef lisp_value * (*lisp_builtin_func)(lisp_runtime*, lisp_scope*, lisp_value*, void*);

/**
 * Create a new ::lisp_builtin from a function pointer, with a given name.
 * @warning Namse of builtins are not garbage collected, since they are almost
 * always static. If you need your name to be dynamically allocated, you'll have
 * to free it after you free the runtime.
 * @param rt runtime
 * @param name name of the builtin. the interpreter will never free the name!
 * @param call function pointer of the builtin
 * @param user a user context pointer which will be given to the builtin
 * @return new builtin object
 */
lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call, void *user);

/**
 * Shortcut to declare a builtin function. Simply takes a function pointer and a
 * string name, and it will internally create the ::lisp_builtin object with the
 * correct name, and bind it in the given scope.
 * @param rt runtime
 * @param scope scope to bind builtin in
 * @param name name of builtin
 * @param call function pointer defining the builtin
 * @param user a user context pointer which will be given to the builtin
 */
void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name,
                            lisp_builtin_func call, void *user);

/**
 * Given a list of arguments, evaluate each of them within a scope and return a
 * new list containing the evaluated arguments. This is most useful for
 * implementing builtin functions.
 * @param rt runtime
 * @param scope scope to evaluate within
 * @param list list of un-evaluated function arguments
 * @return list of evaluated function arguments
 * @retval NULL if an error occured during evaluation
 */
lisp_value *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_value *list);

/**
 * Given a list of ::lisp_value's, evaluate each of them within a scope,
 * returning the last value. This is similar to lisp_eval_list(), but rather
 * than constructing a full list of results, it merely returns the last one. It
 * is used in the ``progn`` builtin, but it also is useful for doing things like
 * evaluating everything in a file or allowing implimenting "implicit progns".
 * @param rt runtime
 * @param scope scope
 * @param l list of expressions to evaluate
 */
lisp_value *lisp_progn(lisp_runtime *rt, lisp_scope *scope, lisp_list *l);

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
 *     R - Rest of arguments
 *
 * As an example, a function which takes an integer and a string, and prints the
 * string N times, might use the format string ``dS``.
 *
 * The remaining variadic arguments are pointers to object pointers, and they
 * will be assigned as each argument is parsed. EG:
 *
 *     lisp_integer *arg1;
 *     lisp_string *arg2;
 *     lisp_get_args(rt, args, "dS", &arg1, &arg2);
 *
 * @note The format code 'R' is special and deserves some more attention. When
 * used, it immediately ends argument processing, so it should only be used at
 * the end of a format string.  It will resolve to the remaining unprocessed
 * arguments as a list, provided that there is at least one (i.e. R will fail if
 * the rest of the args is an empty list).
 *
 * @param rt runtime
 * @param list Argument list to type check and count
 * @param format Format string
 * @param ... Destination pointer to place results
 * @retval 1 on success (true)
 * @retval 0 on failure (false)
 */
int lisp_get_args(lisp_runtime *rt, lisp_list *list, char *format, ...);

/**
 * @}
 * @defgroup embed Embedding API
 * @{
 */

/**
 * Parse a *single* expression from @a input, starting at @a index. Sets the
 * result as a ::lisp_value in @a output. Return the number of bytes parsed from
 * @a input.
 *
 * When a parse error occurs, the return value is negative, and @a output is set
 * to NULL. The error code and line number are set in the runtime, and may be
 * retrieved with lisp_get_error().
 *
 * When the string contains no expression (only whitespace or comments), the
 * return value will still be non-negative. @a output will be set to NULL. This
 * situation is typically not an error, either meaning empty REPL input or the
 * end of the file you are parsing.
 *
 * @param rt runtime to create language objects in
 * @param input string to parse
 * @param index position in @a input to start parsing
 * @param output pointer to ``lisp_value **`` where we store the parsed
 * expression.
 * @return number of bytes processed from @a input
 * @retval -1 when an error occurs during parsing
 */
int lisp_parse_value(lisp_runtime *rt, char *input, int index, lisp_value **output);

/**
 * Parse every expression contained in @a input. Return the parsed code as a
 * list, with the first element being the symbol ``progn``, and the remaining
 * elements being the parsed code. This may be evaluated using lisp_eval().
 *
 * When a parse error occurs, NULL is returned. Note that parse errors typically
 * occur after memory allocation has occurred. Memory allocated by this function
 * is not cleaned up on error, and must be garbage collected.
 *
 * Note that if the string is entirely empty, or only contains comments, then
 * the progn will be empty, which currently causes an exception when evaluated.
 * @param rt runtime
 * @param input string to parse
 * @return the code, fully parsed, within a progn block
 * @retval NULL when an error occurs (see lisp_print_error())
 */
lisp_value *lisp_parse_progn(lisp_runtime *rt, char *input);

/**
 * Parse every expression contained in @a file, and return the parsed code as a
 * ``progn`` block. This function behaves same as lisp_parse_progn(). Additional
 * errors may be raised due to I/O errors on @a file.
 * @param rt runtime
 * @param file file to parse
 * @return the code, fully parsed, within a progn block
 * @retval NULL when an error occurs (see lisp_print_error())
 */
lisp_value *lisp_parse_progn_f(lisp_runtime *rt, FILE *file);

/**
 * Parse a file and evaluate its contents. This is roughly equivalent to:
 * @code
 * lisp_value *progn = lisp_parse_progn_f(rt, scope, input)
 * lisp_eval(rt, scope, progn);
 * @endcode
 * @param rt runtime
 * @param scope scope to evaluate within (usually a default scope)
 * @param input file to load as funlisp code
 * @return the result of evaluating the last item
 * @retval NULL on empty file, or file read error
 */
lisp_value *lisp_load_file(lisp_runtime *rt, lisp_scope *scope, FILE *input);

/**
 * Lookup the symbol ``main`` in the scope, and run it if it exists. Calls the
 * function with a single argument, a ::lisp_list of program arguments. argc and
 * argv should not include the main executable (just the script name and args).
 * @param rt runtime
 * @param scope scope to find main in
 * @param argc number of arguments
 * @param argv NULL-terminated argument list
 * @returns result of evaluation
 * @retval a nil list when there is no main symbol
 * @retval NULL on error
 */
lisp_value *lisp_run_main_if_exists(lisp_runtime *rt, lisp_scope *scope,
                                    int argc, char **argv);

/**
 * Mark an object as still reachable or useful to the program (or you). This can
 * be called several times to mark many objects. Marking objects prevents the
 * garbage collector from freeing them. The garbage collector performs a breadth
 * first search starting from your marked objects to find all reachable language
 * objects. Thus, marking an object like a ::lisp_scope will save all symbols and
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

/**
 * Return @a value, but inside a list containing the symbol ``quote``. When this
 * evaluated, it will return its contents (@a value) un-evaluated.
 *
 * This function is used during parsing, to implement the single-quote syntax
 * feature. For example ``'(a b c)``, evaluates to the list containing a, b,
 * and c, rather than calling a on b and c. This is because the expression is
 * transparently converted to the more verbose ``(quote (a b c))``.
 *
 * @param rt runtime
 * @param value value to return quoted
 * @return value but quoted
 */
lisp_value *lisp_quote(lisp_runtime *rt, lisp_value *value);

/**
 * @}
 * @defgroup error Error Handling
 * @{
 */

enum lisp_errno {
	LE_ERROR=1,  /* a catch-all */
	LE_EOF,      /* end of file while parsing */
	LE_SYNTAX,   /* syntax error */
	LE_FERROR,   /* error reading file */
	LE_2MANY,    /* too many args */
	LE_2FEW,     /* not enough args */
	LE_TYPE,     /* wrong type arg in function */
	LE_NOCALL,   /* not callable */
	LE_NOEVAL,   /* not evaluate-able */
	LE_NOTFOUND, /* not found */
	LE_EXIT,     /* exit the interpreter */

	LE_UNUSED__  /* don't use this, it's just for the trailing comma */
};

/**
 * Raise an error in the interpreter and return NULL.
 *
 * This function is meant to be used within code that implements builtins. When
 * an error condition is reached, functions may simply do something like this:
 *
 * @code
 * if (some_error_condition())
 *     return lisp_error(rt, LE_ERROR, "you broke something");
 * @endcode
 *
 * @param rt runtime
 * @param errno error number, for easy programatic acccess
 * @param message message to show the user
 * @return NULL
 */
lisp_value *lisp_error(lisp_runtime *rt, enum lisp_errno errno, char *message);

/**
 * Dump the execution stack to a file. This is useful if you want to print a
 * stack trace at your current location. This functionality can also be accessed
 * via the ``dump-stack`` builtin function.
 * @param rt runtime
 * @param stack When NULL, the runtime's execution stack is used. When non-NULL,
 * the @a stack argument is used to specify what stack to dump.
 * @param file where to dump stack trace to
 */
void lisp_dump_stack(lisp_runtime *rt, lisp_list *stack, FILE *file);

/**
 * A macro for error checking the return value of a lisp_eval() or lisp_call()
 * function. This will return NULL when its argumnet is NULL, helping functions
 * short-circuit in the case of an error.
 *
 * @code
 * lisp_value *v = lisp_eval(rt, my_code, my_scope);
 * lisp_error_check(v);
 * // continue using v
 * @endcode
 *
 * @param value value to error check
 */
#define lisp_error_check(value) do { \
		if (!value) { \
			return NULL; \
		} \
	} while (0)

/**
 * Prints the last error reported to the runtime, on @a file. If there is no
 * error, this prints a loud BUG message to FILE, indicating that an error was
 * expected but not found.
 * @param rt runtime
 * @param file file to print error to (usually stderr)
 */
void lisp_print_error(lisp_runtime *rt, FILE *file);

/**
 * Returns the error text of the current error registered with the runtime.
 * @param rt runtime
 * @return error string
 */
char *lisp_get_error(lisp_runtime *rt);

/**
 * Returns the error number of the current error registered with the runtime.
 * @param rt runtime
 * @return error number
 */
enum lisp_errno lisp_get_errno(lisp_runtime *rt);

/**
 * Clears the error in the runtime.
 * @param rt runtime
 */
void lisp_clear_error(lisp_runtime *rt);

extern const char * const lisp_version;

/**
 * @}
 */

#endif
