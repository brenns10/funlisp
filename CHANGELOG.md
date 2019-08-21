Changelog
=========

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/). This
project uses [Semantic Versioning](https://semver.org) in the following ways:

- Before 1.0.0, there is no API or language stability - minor or patch releases
  may break anything.
- Since 1.0.0, the embedding API is stable and breaking changes must be made on
  a major release. However, the language itself may go through extension and
  breaking changes.
- At some unknown X.0.0, the language and embedding API will both be subject to
  SemVer.

## Unreleased

## [1.2.0] 2019-08-20

After nearly a year without updates, Funlisp v1.2.0 is released!  This release
brings several new language features and API additions. On top of that, I've
moved the main development Git host to
[Sourcehut](https://git.sr.ht/~brenns10/funlisp/), although the GitHub
repository will continue to act as a mirror. Releases will still have to be
found on GitHub, since Sourcehut does not have a suitable system for that yet.

In a similar vein, we now have a mailing list,
`~brenns10/funlisp-dev@lists.sr.ht`. This will contain version announcements,
and any patches are welcome to be submitted there! For docs on using the list,
see [Sourcehut manual](https://man.sr.ht/lists.sr.ht/). TLDR:

- Email `~brenns10/funlisp-dev+subscribe@lists.sr.ht` with any subject/content
  in order to subscribe.
- Email `~brenns10/funlisp-dev+unsubscribe@lists.sr.ht` to unsubscribe.

You may notice some test posts on the list -- no more will be posted, so it is
safe to subscribe if you're interested.  You need not be a member of Sourcehut
to use the list, submit patches, or browse the repository.

### Added

- `let` statement for binding names to values. Sample usage:

      (define sum-of-squares (lambda (x y)
        (let ((x_sq (* x x))
              (y_sq (* y y)))
             (+ x_sq y_sq))))

      (sum-of-squares 3 4)
      -> 25

- `cond` statement for multi-case conditionals. Sample usage:

      (define get-maintainer (lambda (file)
        (cond
          ((eq? file "Makefile") "Stephen")
          ((eq? file "inc/funlisp.h") "Stephen")
          (1 "Stephen"))))

- Documentation for the `macro` statement which existed in 1.1 is now present on
  the documentation site + manual page.

- Modules!
  - Use `(import module)` to either load the builtin "module" or read
    "module.lisp" and access its defined symbols.
  - C API for creating builtin modules exists, and is documented.
  - First module, `os`, containing `getenv` function, is created. I have not yet
    figured out a solid standard library documentation system, so consider the
    builtin modules and their functions to be provisional.

      (import os)
      (os.getenv "HOME")
      -> "/home/stephen"

### Fixed
- `macro` construct was incorrectly evaluating its arguments prior to execution.
  This change is breaking, but since it reflects major incorrect behavior, it
  will be released in a minor version bump.

## [1.1.0] - 2018-08-22

### Added
- Added `lisp_list_append()` and `lisp_list_set_{left|right}` for mutating and
  constructing lists more easily.

## [1.0.0] - 2018-08-16

First 1.0 release. The API should now be stable enough for you to use!

### Added
- Line comments are now supported via the semicolon (`;`) character
- New builtins:
  * `progn` - allows sequences of multiple expressions to be evaluated
  * `quasiquote` - quote an expression, except for instances of `unquote`
  * `unquote` - embed an evaluated expression within a `quasiquote`
  * `assert` - raise an error if the value of an expression is "false" (the
    integer 0, or not an integer at all)
  * `assert-error` - assert than evaluating an expression results in an error
  * `macro` - similar to lambda, but evaluates to code which is then evaluated
- New syntax:
  * Use backtick to quasiquote, and comma unquote, in the same way that the
    single quote invokes the `quote` builtin
- Added `funlisp` binary, which aims to be a complete, user-friendly repl and
  script runner.
- Added `lisp_version`, a string containing the funlisp version.
- Added support for a `macro` builtin, which is identical to `lambda`, but
  instead returns code which should be evaluated.
- Symbols and strings may now be cached for better memory and runtime
  performance. This done transparently, but must be enabled with
  `lisp_enable_symcache()` and `lisp_enable_strcache()`.
### Changed
- Parsing API is now flexible enough for public users. Removed `lisp_parse()`
  and added `lisp_parse_value()`, as well as `lisp_parse_progn()` and
  `lisp_parse_progn_f()`. **This change is backward incompatible.**
- Error handling has received more love. Errors are now set at the interpreter
  level, with error codes and messages. Codes are useful for programs to
  understand the type of error which occurred, while messages are useful for
  users looking at errors. **This change is backward incompatible.**
- Progns may now be empty
- In stack traces, lambdas and macros now show the first name they were bound
  to, so that you can more easily read them.

## [0.1.0] (BETA) - 2018-06-13

Initial release! This is the first release with a source tarball and the
capability to be installed system-wide. It is a beta release, so the API is not
yet fully stable. Currently included in this release:
- Builtin functions, lambda functions, etc
- User context for runtime and builtin objects
- Full C89 compliance

See the [Github issue tracker](https://github.com/brenns10/funlisp/issues) for
planned features, especially in the
[Initial release milestone](https://github.com/brenns10/funlisp/milestone/2).
