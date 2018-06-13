# funlisp - Makefile
#
# Stephen Brennan <stephen@brennan.io>

# Setting for CFLAGS, CC, and DESTDIR come from Makefile.conf
include Makefile.conf

OBJS=src/builtins.o src/charbuf.o src/gc.o src/hashtable.o src/iter.o \
     src/parse.o src/ringbuf.o src/types.o src/util.o

all: bin/libfunlisp.a FORCE

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

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

clean: FORCE
	rm -rf bin/* src/*.o tools/*.o

FORCE:
	@true

#
# Below are tools mainly meant to be run by developers, using a broader array of
# GNU utilities. They may not be strictly standard.
#
doc: FORCE
	doxygen
	make -C doc html man

clean_doc:
	rm -rf doc/xml
	make -C doc clean

serve_doc: doc
	cd doc/_build/html; python -m http.server --bind 0.0.0.0 8080

# Named differently from Makefile.dep to avoid implicit rules for inclusion.
depend: FORCE
	gcc $(CFLAGS) -MM src/*.c > Makefile.dep

include Makefile.dep
