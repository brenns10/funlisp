# funlisp - Makefile
#
# Stephen Brennan <stephen@brennan.io>

# Setting for CFLAGS, CC, and DESTDIR come from Makefile.conf
include Makefile.conf

OBJS=src/builtins.o src/charbuf.o src/gc.o src/hashtable.o src/iter.o \
     src/parse.o src/ringbuf.o src/types.o src/util.o

# https://semver.org
VERSION=0.1.0

all: bin/libfunlisp.a FORCE

.c.o:
	$(CC) $(CFLAGS) -DFUNLISP_VERSION=\"$(VERSION)\" -c $< -o $@

bin/libfunlisp.a: $(OBJS)
	ar rcs $@ $^

bin/repl: tools/repl.o bin/libfunlisp.a
	$(CC) -ledit $^ -o $@

bin/hello_repl: tools/hello_repl.o bin/libfunlisp.a
	$(CC) -ledit $^ -o $@

bin/runfile:  tools/runfile.o bin/libfunlisp.a
	$(CC) $^ -o $@

bin/call_lisp: tools/call_lisp.o bin/libfunlisp.a
	$(CC) $^ -o $@

bin/funlisp: tools/funlisp.o bin/libfunlisp.a
	$(CC) -ledit $^ -o $@

clean: FORCE
	rm -rf bin/* src/*.o tools/*.o

# Meant to be run after downloading the source tarball. This has an un-expressed
# dependency on `man/funlisp.3`, since the source tarball includes generated
# documentation, while the git repository does not. Leaving that dependency out
# allows the source tarball to be installed on a wide variety of POSIX compliant
# machines. Depends on the variables (set in Makefile.conf):
#   DESTDIR: specifies alternative root of installation, typically /
#   PREFIX: typically usr
# Taken together, the usual location for the binary is /usr/lib/libfunlisp.a
install: bin/libfunlisp.a
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 bin/libfunlisp.a $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 inc/funlisp.h $(DESTDIR)$(PREFIX)/include/
	@# Documentation installed only if it has been generated.
	if [ -f man/funlisp.3 ]; then \
	  install -d $(DESTDIR)$(PREFIX)/share/man/man3/ ; \
	  install -m 644 man/funlisp.3 $(DESTDIR)$(PREFIX)/share/man/man3/ ; \
	fi

# For uninstallation of the library. Should be kept in sync with the above
# installation files. Depends on the same variables.
uninstall: FORCE
	@echo 'Sorry to see you go!'
	rm $(DESTDIR)$(PREFIX)/lib/libfunlisp.a
	rm $(DESTDIR)$(PREFIX)/include/funlisp.h
	rm -f $(DESTDIR)$(PREFIX)/share/man/man3/funlisp.3

FORCE:
	@true

#
# Below are tools mainly meant to be run by developers, using a broader array of
# GNU utilities. They may not be strictly standard.
#
doc: FORCE
	doxygen
	sphinx-build -d doc/.doctrees -b html doc html
	sphinx-build -d doc/.doctrees -b man doc man

clean_doc:
	rm -rf doc/xml man html
	make -C doc clean

# Create a distribution package, with generated manual and html included. The
# --transform argument ensures that when the tarball is extracted, all files are
# within a subdirectory.
package: FORCE clean doc
	tar --transform='s/^/funlisp-$(VERSION)\//' -cvf funlisp-$(VERSION).tar.gz \
		LICENSE.txt README.md INSTALL.md CHANGELOG.md \
		Makefile Makefile.conf Makefile.conf.dev Makefile.dep \
		Doxyfile doc/*.rst doc/Makefile doc/conf.py doc/requirements.txt \
		html man \
		inc/*.h src/*.c src/*.h tools/*.c scripts/*.lisp bin/.gitkeep

serve_doc: doc
	cd doc/_build/html; python -m http.server --bind 0.0.0.0 8080

# Named differently from Makefile.dep to avoid implicit rules for inclusion.
depend: FORCE
	gcc $(CFLAGS) -MM src/*.c > Makefile.dep

include Makefile.dep
