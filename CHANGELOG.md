Changelog
=========

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/). This
project will use [Semantic Versioning](https://semver.org) for non-beta releases
(that is, starting with 1.0.0). During the beta phase (right now) breaking
changes may be made on minor version bumps, and feature additions / bug-fixes
will be made on patch versions.

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
