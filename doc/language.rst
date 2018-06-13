Language Guide
==============

Hello World
-----------

As any good programming language documentation should, let's start with hello
world:

.. code:: lisp

  (define main
    (lambda (args)
      (print "hello world")))

Here we see many of the normal expectations from a hello world: a function
declaration and a print statement. ``define`` binds a name to any value you'd
like. In this case, we're binding the symbol ``main`` to be a function.

Functions are declared using the ``lambda`` statement. They are enclosed in
parentheses, and first contain a list of arguments, then an expression that is
their implementation. Our ``main`` function simply prints a string.  We can run
this script using the ``bin/runfile`` utility bundled with funlisp:

.. code:: bash

  $ make bin/runfile
  # some make output
  $ bin/runfile scripts/hello_world.lisp
  hello world

Comments
--------

Line comments are created in funlisp by starting them with a semicolon (``;``).
Everything after that, until the end of the line, is ignored by the parser. Here
are some examples:

.. code:: lisp

  ; i'm a comment
  (print "i'm some code" ; with a comment in the middle
  )

Types
-----

To accelerate our learning, try compiling the REPL. You might need to install
libedit to build it.

.. code:: bash

  $ make bin/repl
  # some make output
  $ bin/repl
  >

Like most REPLs, this takes a line at a time, executes it, and prints the
result. We'll use this for demonstrating some of funlisp's types.

.. code::

  > 5
  5
  > "blah"
  blah

The two most obvious data types are the integer and string, as you can see
above.

.. code::

  > 'hello
  hello
  > '(1 2 3)
  (1 2 3 )

Above are two less obvious types, the "symbol" and the "list". Symbols are like
strings, but they are used to bind names to values in lisp. Lists are the
fundamental data structure of funlisp, used to hold both data and code! Both of
those had quotes in front of them. Quoting in this way prevents funlisp from
evaluating them, since symbols and lists behave a bit differently when they're
evaluated:

.. code::

  > hello
  error: symbol not found in scope

The symbol was looked up in the scope, but it turns out that there's no
``hello`` in scope. However, we know several things that are in the scope, from
our hello world program:

.. code::

  > define
  <builtin function define>
  > lambda
  <builtin function lambda>
  > print
  <builtin function print>

Neat! The symbols are names for builtin functions.

Lists behave in a special way when evaluated too. Funlisp tries to "call" the
first item on the list, with the other items as arguments. We saw this with
hello world as well:

.. code::

  > (1 2)
  error: not callable!
  > (print 2)
  2

Turns out that "1" is not callable, but the function attached to the ``print``
symbol is!

Integer Functions
-----------------

So, we've seen some neat tricks with the four basic builtin types. Now let's see
how to manipulate integers:

.. code::

  > (+ 1 1)
  2
  > (- 5 1)
  4
  > (/ 4 3)
  1
  > (* 2 2)
  4

Those basic arithmetic functions behave like any other function call. They look
a bit odd because we expect arithmetic operators to be in the middle of an
expression, but you'll get used to it!

.. code::

  > (= 5 5)
  1
  > (> 5 6)
  0
  > (<= 4 5)
  1

Comparison operators look like that too. They return integers, which are used
for conditionals in funlisp the same way that C does.

Control Flow
------------

Speaking of control-flow, funlisp has a handy if statement:

.. code::

  > (if (= 5 4) (print "impossible") (print "boring"))
  boring

Since we try to make everything in funlisp into an expression, if statements
must have both a "value if true" and a "value if false". You cannot leave out
the else.

Funlisp doesn't currently have any form of iteration. However, it supports
recursion, which is a very powerful way of iterating, and handling objects like
lists.

Functions and Recursion
-----------------------

We've already seen the lambda syntax of creating functions for our hello world.
Now let's check out some others:

.. code::

  > (define double (lambda (x) (* 2 x)))
  <lambda function>
  > (double 2)
  4

We can recursively call our own function, for great good:

.. code:: lisp

  (define factorial
    (lambda (x)
      (if (= 0 x)
        1
        (* x (factorial (- x 1))))))

We can also use that capability to process a list of elements:

.. code:: lisp

  (define increment-all
    (lambda (x)
      (if (null? x)
        '()
        (cons (+ 1 x) (increment-all (cdr x))))))

Oops, looks like I've introduced you to cons and cdr. These are from a family of
list processing functions that do the following:

- ``(cons x l)`` - put x at the beginning of list l
- ``(car l)`` - return the first item in l
- ``(cdr l)`` - return the elements after the first one in l

Higher Order Functions
----------------------

Now that we've incremented each item in a list, what if we want to decrement?
We'd have to rewrite the whole function again, replacing the plus with a minus.
Thankfully, we can do a better job, using ``map``:

.. code:: lisp

  (define increment-all (lambda (l) (map (lambda (x) (+ 1 x) l))))

Map is a function which takes another function, as well as a list of items, and
applies it to each item, returning the list of results.

We also have access to the reduce function, which applies that function to the
list in pairs:

.. code::

  > (reduce + '(1 2 3))
  6

The End
-------

This is the end of our short guide to funlisp. Hopefully as time goes by, the
language will grow, and maybe even obtain a standard library! But for now,
funlisp remains sleek and small.
