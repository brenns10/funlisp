Embedding
=========

Funlisp is simply a static library. The 'fun' part comes when it is linked with
a program that can make creative use of a scripting language. Funlisp comes with
some useful example programs - a REPL (read-eval-print loop) and a script
runner. In this section, we'll go over some basic concepts of funlisp, describe
the implementation of these tools, and then go into how your program can
integrate with the library.

Basic Components
----------------

To use the lisp interpreter, there are a few basic concepts to understand.

The interpreter has a :c:type:`lisp_runtime` object associated with it. It holds
all sorts of contextual information about the interpreter, especially having to
do with garbage collection. You need to have an instance of a runtime in order
use most of the rest of the library. You can create a runtime with
:c:func:`lisp_runtime_new()` and once initialized, you must destroy it with
:c:func:`lisp_destroy()`.

.. warning::

  Destroying a runtime also ends up garbage collecting all language objects
  created within that runtime, so if you want to access language objects, do it
  before destroying the runtime.

In order to run any code, you will need to have a global :c:type:`lisp_scope`.
This object binds names to funlisp values, including several of the critical
built in functions and constructs of the language. You can use the function
:c:func:`lisp_new_default_scope()` to create a scope containing all the default
language constructs (these are pretty important). If you ever need a new, empty
scope, you can create it with :c:func:`lisp_new_empty_scope()`.

Objects contained within the scope (and in fact, the scope itself) are all of
type :c:type:`lisp_value` - aka a funlisp object. This means they all share some
common fields at their head, they all support some common operations (such as
printing), and they are all managed by the garbage collector. So, you won't need
to manually free a scope. Instead, use the garbage collector.

Speaking of this garbage collector, how does it work? Funlisp uses a mark and
sweep garbage collector. This means that every so often the application must
pause the program, mark all reachable language objects, and free everything that
is unreachable. To do this, you need a "root set" of objects, which is typically
your global scope. You should call :c:func:`lisp_mark()` on this root set,
followed by :c:func:`lisp_sweep()` on the runtime to free up all objects
associated with your runtime, which are not reachable from your root set.

.. warning::

  The garbage collector can make it easy to shoot yourself in the foot. It has
  no qualms with freeing a :c:type:`lisp_value` that you'll be using on the very
  next line of code. So be sure to explicitly mark not only the objects that
  your scripts will need (this is usually just the scope), but also the objects
  that your C code would like to access, when you run the GC.

The REPL
--------

With this knowledge, a REPL is pretty easy to make! Here is an outline of the
steps:

1. First, create a language runtime and a global scope.
2. Read a line of input.
3. Parse the input. Parsed code is simply a :c:type:`lisp_value` like any other
   language object.
4. Evaluate the input within the global scope.
5. If an error occurred, print it and continue. If nothing of interest is
   returned, do nothing. Otherwise, print the output, and a trailing newline.
6. Mark everything in scope, then sweep unreachable objects.
7. Repeat steps 2-7 for each line of input.
8. Destroy the language runtime to finish cleaning up memory.

Here is some basic code that demonstrates embedding a simple lisp interpreter,
without any custom functions. It uses the ``editline`` implementation of the
``readline`` library for reading input (and allowing line editing).

.. literalinclude:: ../tools/repl.c
  :language: C

Notice here that :c:func:`lisp_eval()` returns NULL in case of an error. If that
happens, then you can use :c:func:`lisp_print_error()` to print a user-facing
error message, and :c:func:`lisp_clear_error()` to clear the error from the
interpreter state.

The Script Runner
-----------------

Running a script is a slightly different story. Scripts generally define
functions, including a main function, and then want to be executed with some
command line arguments. So after reading and executing all the lines of code in
a file, you'll want to execute the main function. We can use a nice utility for
that, :c:func:`lisp_run_main_if_exists()`. What's more, the
:c:func:`lisp_parse()` function used in the REPL can only parse a single
expression. We have to use a more powerful option that can handle a whole file
full of multiple expressions: :c:func:`lisp_load_file()`.

Below is the source of the bundled script runner:

.. literalinclude:: ../tools/runfile.c
  :language: C

Calling C Functions From Lisp
-----------------------------

Typically, an embedded interpreter will not be of much use to your application
unless you can call into your application, or your application can call into the
interpreter.  The most straightforward way to add your own functionality to the
interpreter is by writing a "builtin" - a C function callable from lisp.
Builtins must have the following signature:

.. code:: C

   lisp_value *lisp_builtin_somename(lisp_runtime *rt,
                                     lisp_scope *scope,
                                     lisp_value *arglist,
                                     void *user);

The scope argument contains the current binding of names to values, and the
arglist is a list of arguments to your function, which **have not been
evaluated**. These arguments are essentially code objects. You'll almost always
want to evaluate them all before continuing with the logic of the function. You
can do this individually with the :c:func:`lisp_eval()` function, or just
evaluate the whole list of arguments with the :c:func:`lisp_eval_list()`
function.

.. warning::

  As we've noticed in the previous example programs, evaluating code can return
  ``NULL`` if an error (e.g. an exception of some sort) occurs. A well-behaved
  builtin will test the result of all calls to :c:func:`lisp_eval()` and
  :c:func:`lisp_call()` using the macro ``lisp_error_check()`` in order to
  propagate those errors back to the user. :c:func:`lisp_eval_list()` propagates
  errors back, and so it should be error checked as well.

The one exception to evaluating all of your arguments is if you're defining some
sort of syntactic construct. An example of this is the if-statement. The if
statement looks like ``(if condition expr-if-true expr-if-false)``. It is
implemented as a builtin function, which first evaluates the condition. Then,
only the correct expression is evaluated based on that condition.

Finally, when you have your argument list, you could verify them all manually,
but this process gets annoying very fast. To simplify this process, there is
:c:func:`lisp_get_args()`, a function which takes a list of (evaluated or
unevaluated) arguments and a format string, along with a list of pointers to
result variables. Similar to ``sscanf()``, it reads a type code from the format
string and attempts to take the next object off of the list, verify the type,
and assign it to the current variable in the list. The current format string
characters are:

- ``d``: for integer
- ``l``: for list
- ``s``: for symbol
- ``S``: for string
- ``o``: for scope
- ``b``: for builtin
- ``t``: for type
- ``*``: for anything

So, a format string for the plus function would be ``"dd"``, and the format
string for the ``cons`` function is ``"**"``, because any two things may be put
together in an s-expression. If nothing else, the :c:func:`lisp_get_args()`
function can help you verify the number of arguments, if not their types. When
it fails, it returns false. It sets an internal interpreter error depending on
what happened (too many arguments, not enough, types didn't matche, etc). You
can handle this by simply returning NULL from your builtin.  If argument parsing
doesn't fail, your function is free to do whatever logic you'd like.

Finally, note that the signature includes a ``void *user`` parameter. This "user
context" is specified when you register the builtin function, and passed back to
you at runtime.

Basics of Lisp Types
--------------------

In order to write any interesting functions, you need a basic idea of how types
are represented and how you can get argument values out of the
:c:type:`lisp_value` objects. This is not a description of the type system (a
future page in this section will cover that), just a list of available types and
their values.

.. warning::

  Currently, the structs defining these types are not in the public header file,
  ``funlisp.h``. We're still working on the best way to expose types to the API.

The current types (that you are likely to use) are:

- ``lisp_list``: contains a ``left`` and a ``right`` pointer.

  - ``left`` is usually a value of the linked list, and ``right`` is usually the
    next list in the linked list. However this isn't necessarily the case,
    because this object really represents an s-expression, and the right value
    of an s-expression doesn't have to be another s-expression.
  - The empty list is a special instance of ``lisp_list``. You can get a new
    reference to it with ``lisp_nil_new()`` and you can check if an object is
    nil by calling ``lisp_nil_p()``.
  - You can find the length of a list by using ``lisp_list_length()``.

- ``lisp_symbol``: type that represents names. Contains ``sym``, which is a
  ``char*``.
- ``lisp_integer``: contains attribute ``x``, an integer. Yes, it's allocated on
  the heap.  Get over it.
- ``lisp_string``: another thing similar to a symbol in implementation, but this
  time it represents a language string literal. The ``s`` attribute holds the
  string value.

There are also types for builtin functions, lambdas, scopes, and even a type for
types!  But you probably won't use them in your average code.

Adding Builtins to the Scope
----------------------------

Once you have written your functions, you must finally add them to the
interpreter's global scope. Anything can be added to a scope with
``lisp_scope_bind()``., but the name needs to be a ``lisp_symbol`` instance and
the value needs to be a ``lisp_value``. To save you the trouble of creating
those objects, you can simply use ``lisp_scope_add_builtin()``, which takes a
scope, a string name, a function pointer, and a user context pointer.

Here is a code example that puts all of this together, based on the REPL given
above.

.. literalinclude:: ../tools/hello_repl.c
  :language: C

In this example, we've added a builtin function, defined at (1). This function
takes a string, and prints a greeting message. It uses its "user context" object
as a string, to introduce itself. During startup, we register the builtin
function once, with name "hello" (2). We provide it with a user context of "a
computer." We register it again at (3), with the name "hello_from_stephen", and
the user context "Stephen."

An example session using the builtin shows how the functions may be called from
the REPL, and how the user context objects affect the builtin:

.. code::

   > (hello "Stephen")
   Hello, Stephen! I'm a computer.
   > (hello_from_stephen "computer")
   Hello, computer! I'm Stephen.
   > (hello 1)
   error: expected a string!
   > (hello 'Stephen)
   error: expected a string!

Calling Lisp Functions From C
-----------------------------

Just like it's possible to call C from lisp, you can call lisp from C. You've
already seen plenty of examples of this, in the runfile tool. The runfile tool
uses a helper function called :c:func:`run_main_if_exists()`, but here's how it
works.

1. First, you need to have a runtime and scope. If you're working in a builtin,
   you may have been given one. Otherwise, you'll probably have to create a
   default scope, and probably load in some user code too.
2. Second, you need to look up the function you want to call. Typically, you'll
   look it up in your scope using :c:func:`lisp_scope_lookup()`.
3. Third, you need to build your arguments. Arguments are always a list of
   **un-evaluated** values. Since the lisp function will evaluate its arguments,
   you need to make sure they will end up being what you intended. For example,
   evaluating a list will result in calling the first item with the rest of the
   items as arguments. A fool proof way of ensuring that your data makes it
   thruogh unscathed is to quote it using :c:func:`lisp_quote()`.
4. Fourth, you should use :c:func:`lisp_call()` to call the function with your
   arguments.

Here's a full example!

.. literalinclude:: ../tools/call_lisp.c
  :language: C

A few things to note about this:

- We wrapped the lisp function in a C one. This is nice for getting rid of some
  complexity.
- There was no need to quote the integer, because integers evaluate to
  themselves.
- We had to build the argument list from scratch :(

This program produces exactly the output you'd expect:

.. code::

  $ bin/call_lisp
  (double_or_square 5) = 25
  (double_or_square 7) = 49
  (double_or_square 9) = 81
  (double_or_square 11) = 22
  (double_or_square 13) = 26

User Contexts
-------------

Usually, an application will want to have access to some of its own data. To
that end, the embedding application may associate a "user context" with the
:c:type:`lisp_runtime` using :c:func:`lisp_runtime_set_ctx()`. This context may
be some global state, etc. Later (e.g. within a builtin function), the context
may be retrieved with :c:func:`lisp_runtime_get_ctx()`.

Furthermore, each builtin function may associate itself with a user context
object as well (provided upon registration). This allows the same C function to
be registered multiple times as multiple Funlisp functions. It also allows the C
function to access additional context, which it may not have been able to get
through the context object attached to the runtime.

Building Lists
--------------

Many parts of the funlisp API require constructing lists. As has been mentioned
earlier, funlisp "list" types are singly linked lists. Each node's ``left``
pointer points to the data contained by the node, and the ``right`` pointer
points to the next node in the list. ``nil``, returned by
:c:func:`lisp_nil_new()`, is the empty list, and every list is terminated by it.

You have several options available to you for constructing lists. First is
:c:func:`lisp_list_new()`, which allows you to create a new list given a left
and a right pointer. This may be used for constructing single list nodes. It may
also be used to construct a list in reverse, starting with the last node and
continuing to the first one.  You may also directly set the left and right
pointer of a list node, using :c:func:`lisp_list_set_left()` and
:c:func:`lisp_list_set_right()` respectively.

.. warning::

   Note that lisp lists are not mutable. In Lisp, any modification to list
   results in a new one. Thus, you should never modify list nodes, unless you
   have just created them yourself and are constructing a full list.

   Also note that lisp lists' ``left`` and ``right`` pointers should never
   contain ``null`` -- this will certainly cause a segmentation fault when they
   are used. However, you can set these pointers to ``null`` during the
   construction of a list, so long as the end result is a valid list with no
   nulls.

The simplest option is :c:func:`lisp_list_append()`. This function allows you to
construct a list forwards (rather than reverse). It requires double pointers to
the head and tail of the list (where tail is the last non-nil item in the list).
See the generated documentation for full information, but below is a fully
working example of using it:

.. literalinclude:: ../tools/example_list_append.c
  :language: C

Finally, there are a few specialized options for constructing lists.
:c:func:`lisp_singleton_list()` allows you to construct a list with one item.
:c:func:`lisp_list_of_strings()` is useful for converting the argv and argc of a
C main function into program arguments for a funlisp program.

Advanced Topics
---------------

From here, you should be equipped to get a fair amount out of funlisp. If you're
looking for more, check the source code! In the future, I hope to write some
documentation on implementation concepts to make this easier.
