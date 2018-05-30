Language Reference
==================

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
