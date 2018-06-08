# funlisp - Makefile
#
# Stephen Brennan <stephen@brennan.io>

.PHONY: all clean doc

CC=gcc
CFLAGS= -std=c89 -Wall -Wextra -fPIC -Iinc -c
SRCS=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c,obj/%.o,$(SRCS))

# Debug mode (make D=1): adds "debugging symbols" etc
D=0
ifeq ($(D),1)
CFLAGS += -g -DDEBUG
endif

all: bin/libfunlisp.a

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: tools/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bin/libfunlisp.a: $(OBJS)
	ar rcs $@ $^

bin/repl: obj/repl.o bin/libfunlisp.a
	$(CC) -ledit $^ -o $@

bin/hello_repl: obj/hello_repl.o bin/libfunlisp.a
	$(CC) -ledit $^ -o $@

bin/runfile:  obj/runfile.o bin/libfunlisp.a
	$(CC) $^ -o $@

bin/call_lisp: obj/call_lisp.o bin/libfunlisp.a
	$(CC) $^ -o $@

clean:
	rm -rf obj/* bin/* doc/xml
	make -C doc clean

doc:
	doxygen
	make -C doc html

serve_doc: doc
	cd doc/_build/html; python -m http.server --bind 0.0.0.0 8080

depend: $(SRCS) src/*.h inc/*.h tools/*.c
	$(CC) $(CFLAGS) -MM $(SRCS) | sed 's!^\(.*\):!obj/\1:!' > depend

include depend
