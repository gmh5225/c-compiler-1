CC     := gcc
CFLAGS := -std=c17
CFLAGS += -pedantic-errors
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -O2

.PHONY: all
all: main

.PHONY: test
test: main test.sh Makefile
	-./test.sh

.PHONY: clean
clean:
	-rm -f main main.o tokenize.o parse.o type.o codegen.o
	-rm -f tmp tmp.s

main: main.o tokenize.o parse.o type.o codegen.o Makefile
	$(CC) -o $@ $(filter-out Makefile, $^)

main.o: main.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

tokenize.o: tokenize.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

parse.o: parse.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

type.o: type.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

codegen.o: codegen.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<
