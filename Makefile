WFLAGS= \
        -Wmissing-field-initializers -Wunused-parameter -Wold-style-definition \
        -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls \
        -Wmissing-noreturn -Wshadow -Wpointer-arith -Wcast-align \
        -Wwrite-strings -Winline -Wformat-nonliteral -Wformat-security \
        -Wswitch-default -Winit-self -Wnested-externs \
        -Wmissing-include-dirs -Wundef -Wmissing-format-attribute
WFLAGS2=-Wall -Wextra \
        -Wswitch-enum -Wswitch \
        -Waggregate-return -Wstrict-prototypes

CFLAGS=-g -std=gnu99 -Iinclude -Iinclude/priv $(WFLAGS)
LDFLAGS=-g

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard include/*.h)

SUBDIRS=tests examples
.PHONY: $(SUBDIRS)

all: libdbrew.a $(SUBDIRS)

libdbrew.a: $(OBJS)
	ar rcs libdbrew.a $(OBJS)

test: libdbrew.a
	$(MAKE) test -C tests

examples:
	cd examples && $(MAKE)

clean:
	rm -rf *~ *.o $(OBJS) libdbrew.a
	$(MAKE) clean -C tests
	cd examples && make clean
