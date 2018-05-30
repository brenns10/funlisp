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
5. Print the output, and a trailing newline.
6. Mark everything in scope, then sweep unreachable objects.
7. Repeat steps 2-7 for each line of input.
8. Destroy the language runtime to finish cleaning up memory.

Here is some basic code that demonstrates embedding a simple lisp interpreter,
without any custom functions. It uses the ``editline`` implementation of the
``readline`` library for reading input (and allowing line editing).

.. literalinclude:: ../tools/repl.c
  :language: C

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
                                     lisp_value *arglist);

The scope argument contains the current binding of names to values, and the
arglist is a list of arguments to your function, which **have not been
evaluated**. These arguments are essentially code objects. You'll almost always
want to evaluate them all before continuing with the logic of the function. You
can do this individually with the :c:func:`lisp_eval()` function, or just
evaluate the whole list of arguments with the :c:func:`lisp_eval_list()`
function.

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
- ``e``: for error
- ``b``: for builtin
- ``t``: for type
- ``*``: for anything

So, a format string for the plus function would be ``"dd"``, and the format
string for the ``cons`` function is ``"**"``, because any two things may be put
together in an s-expression. If nothing else, the :c:func:`lisp_get_args()`
function can help you verify the number of arguments, if not their types. When
it fails, it returns false, which you should typically handle by returning an
error (:c:func:`lisp_error_new()`). If it doesn't fail, your function is free to
do whatever logic you'd like.

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
- ``lisp_error``: similar to a symbol in implementation, but represents an
  error. Has the attribute ``message`` which contains the error message. Create
  a new one with ``lisp_error_new(message)``.
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
scope, a string name, and a function pointer.

Here is a code example that puts all of this together, based on the REPL given
above.

.. literalinclude:: ../tools/runfile.c
  :language: C

An example session using the builtin:

.. code::

   > (hello "Stephen")
   Hello, Stephen!
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

Advanced Topics
---------------

From here, you should be equipped to get a fair amount out of funlisp. If you're
looking for more, check the source code! In the future, I hope to write some
documentation on implementation concepts to make this easier.
