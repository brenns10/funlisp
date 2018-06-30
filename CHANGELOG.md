Changelog
=========

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/). This
project will use [Semantic Versioning](https://semver.org) for non-beta releases
(that is, starting with 1.0.0). During the beta phase (right now) breaking
changes may be made on minor version bumps, and feature additions / bug-fixes
will be made on patch versions.

## [Unreleased]
### Added
- Line comments are now supported via the semicolon (`;`) character
- Implement `progn` builtin, allowing sequences of multiple expressions to be
  evaluated.
- Added `funlisp` binary, which aims to be a complete, user-friendly repl and
  script runner.
- Added `lisp_version`, a string containing the funlisp version.
### Changed
- Parsing API is now flexible enough for public users. Removed `lisp_parse()`
  and added `lisp_parse_value()`, as well as `lisp_parse_progn()` and
  `lisp_parse_progn_f()`.
- Error handling has received more love. Errors are now set at the interpreter
  level, with error codes and messages. Codes are useful for programs to
  understand the type of error which occurred, while messages are useful for
  users looking at errors.

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
