Installation
============

First, configure your installation. `Makefile.conf` contains several important
variables used by the Makefile for building and installing funlisp. The default
one is very cross-platform compatible, but you may want more options enabled
(e.g. for development).

You may install from a Git repo checkout, or from a source tarball. The source
tarball contains pre-built documentation (HTML and manual pages), and thus has
no dependencies other than standard Make and compiler utilities. The git
checkout requires additional tools to be installed, but is smaller.

From Git
--------

If you have a Git checkout, simply run:

    $ make
    $ sudo make install

This will install, but without any manual page. The manual page must be
generated via Sphinx and Doxygen. Assuming you have them installed, the
installation sequence would be:

    $ make
    $ make doc
    $ sudo make install

From Source Tarball
-------------------

If you have downloaded and extracted a source tarball, simply run:

    $ make
    $ sudo make install

This will include manual pages.
