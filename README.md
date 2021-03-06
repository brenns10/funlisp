funlisp
=======

A fun implementation of a lisp-like language, made just for embedding into your
project! The primary goals of this project are ease of embedding, good
portability, and excellent documentation. Check out the [documentation][] or
grab the source and give it a try:

    $ make bin/funlisp
    $ bin/funlisp
    > (+ 5 5)
    10
    > ^D
    $ bin/funlisp scripts/hello_world.lisp
    hello world

Since funlisp is aimed at being easily embeddable, the [tools/](tools/)
directory is full of sample C programs using the library. The `funlisp`
interpreter and script runner is an example of such a program. There are simple
REPL's and script runners as well as examples of function calls between C and
lisp.

[Read the Docs!][documentation]

Installation
------------

I still consider this project to be in a beta state. Some small breaking changes
are still happening to the API, but they are getting smaller. I hope to cut a v1
release soon, but in the meantime feel free to build a copy and use it locally
in your project.

You can also install system-wide from source - see [INSTALL.md](INSTALL.md).

Distribution packages don't exist yet, but when they do, they will be listed
here.

Contributing
------------

I'm hoping to build a clean and understandable lisp. Your contributions, in
terms of reproducible bug reports or code submissions, are welcome. Please feel
free to submit a bug report on Github or via email (see the git logs for my
email address). Patch submissions are welcome again over Github, or via email
(`man git send-email`).

This project is licensed under Revised BSD - see the LICENSE file for details.

History
-------

This is part of a [long](https://github.com/brenns10/lisp)
[line](https://github.com/brenns10/libstephen/tree/master/src/lisp) of lisp
interpreters I've written. Hopefully they're getting better, not worse :P

[documentation]: https://funlisp.readthedocs.io
