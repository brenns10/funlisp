funlisp
=======

Another lisp implementation, this time aiming to be self-contained, portable,
well documented, and easy to embed and use.  This implementation is derived from
[libstephen lisp][], which is in turn built off the experience of an earlier
[attempt][lisp v1]. Try out the repl with:

    make bin/repl
    bin/repl
    > (+ 5 5)
    10

Since funlisp is aimed at being easily embeddable, the [tools/](tools/)
directory is full of sample programs using the library. You can also check out
the documentation on [Read The Docs](https://funlisp.readthedocs.io) or within
the [source](doc/index.rst).

There are sample scripts within [scripts/](scripts/). You can run them with:

    make bin/runfile
    bin/runfile scripts/hello_world.lisp

[libstephen lisp]: https://github.com/brenns10/libstephen/tree/master/src/lisp
[lisp v1]: https://github.com/brenns10/lisp
