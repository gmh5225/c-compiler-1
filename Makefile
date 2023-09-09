CC     := gcc
CFLAGS := -std=c17
CFLAGS += -pedantic-errors
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -O2

.PHONY: all
all: main

.PHONY: test
test: test.sh main
	-./test.sh

.PHONY: clean
clean:
	-rm -f main main.o tokenize.o parse.o codegen.o
	-rm -f tmp tmp.s

main: main.o tokenize.o parse.o type.o codegen.o
	$(CC) -o $@ $^

main.o: main.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<

tokenize.o: tokenize.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<

parse.o: parse.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<

type.o: type.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<

codegen.o: codegen.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<
