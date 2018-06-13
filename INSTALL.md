Installation
============

You may install from a Git repo checkout, or from a source tarball.

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
