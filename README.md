funlisp
=======

Another lisp implementation, this time aiming to be self-contained, portable,
well documented, and easy to embed and use.  This implementation is derived from
[libstephen lisp][], which is in turn built off the experience of an earlier
[attempt][lisp v1].

Public interface with some documentation is in [inc/funlisp.h](inc/funlisp.h),
and you can build the library by simply running `make`. You'll find the result
at `bin/libfunlisp.a`.

Sample applications embedding funlisp are located in [tools/](tools/).  To
build, e.g. `tools/repl.c`, you can do `make bin/repl`.

There are sample scripts within [scripts/](scripts/). You can run them with:

    make bin/runfile
    bin/runfile scripts/hello_world.lisp

[libstephen lisp]: https://github.com/brenns10/libstephen/tree/master/src/lisp
[lisp v1]: https://github.com/brenns10/lisp
