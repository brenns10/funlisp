Welcome to funlisp's documentation!
===================================

funlisp is a small, simple, easy-to-embed scripting language written in C. It is
a dialect of lisp, but doesn't adhere to any particular standard. This
documentation contains information for those looking to learn the language, as
well as those looking to embed and extend it.

You can get started by cloning the repository from GitHub, and then compiling
the REPL:

.. code::

    $ git clone https://github.com/brenns10/funlisp
    $ make bin/repl
    > (define hello (lambda () (print "hello world")))
    <lambda function>
    > (hello)
    hello world

From there, you can evaluate some arithmetic and write some lambdas! See the
table of contents below for guidance on the language, and how to embed the
interpreter in your program. There is also some (limited) documentation on the
internals of the interpreter, to help interested readers of my code (and
probably me someday).

Visit the repository_ for more information, to view the source, to report bugs,
or even to begin contributing!

.. _repository: https://github.com/brenns10/funlisp

.. toctree::
  :maxdepth: 2
  :caption: Contents:

  language.rst
  embedding.rst
  api.rst
  advanced.rst


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
