#
# A simple Makefile to build the shell
#
LDFLAGS=
LDLIBS=-ll -lreadline
# The use of -Wall, -Werror, and -Wmissing-prototypes is mandatory 
# for this assignment
CFLAGS=-Wall -Werror -Wmissing-prototypes -g -O2
#YFLAGS=-v
YACC=bison

SOURCES=$(filter-out cush.c,$(wildcard *.c **/*.c))
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
HEADERS=$(patsubst %.o,%.h,$(OBJECTS))

default: cush

$(OBJECTS) cush.o: $(HEADERS)

# build scanner and parser
shell-grammar.o: shell-grammar.y shell-grammar.l $(HEADERS)
	$(LEX) $(LFLAGS) $*.l
	$(YACC) $(YFLAGS) $<
	$(CC) -Dlint -c -o $@ $(CFLAGS) $*.tab.c
	rm -f $*.tab.c lex.yy.c

# build the shell
cush: $(OBJECTS) cush.o $(HEADERS) shell-grammar.o
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) cush.o shell-grammar.o $(OBJECTS) $(LDLIBS)

clean:
	rm -f $(OBJECTS) cush cush.o shell-grammar.o \
		core.* tests/*.pyc .sw*

