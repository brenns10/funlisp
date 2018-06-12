funlisp
=======

A fun implementation of a lisp-like language, made just for embedding into your
project! The primary goals of this project are ease of embedding, good
portability, and excellent documentation. Check out the [documentation][] or
grab the source and give it a try:

    $ make bin/repl
    $ bin/repl
    > (+ 5 5)
    10

Since funlisp is aimed at being easily embeddable, the [tools/](tools/)
directory is full of sample C programs using the library. There are also sample
scripts within [scripts/](scripts/). You can run them with the `runfile` tool:

    $ make bin/runfile
    $ bin/runfile scripts/hello_world.lisp

[Read the Docs!][documentation]

Using
-----

I still consider this project to be in a beta state. Some small breaking changes
are still happening to the API, but they are getting smaller. I hope to cut a v1
release soon, but in the meantime feel free to build a copy and use it in your
library :)

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
