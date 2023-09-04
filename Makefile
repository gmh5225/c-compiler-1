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

main: main.o tokenize.o parse.o codegen.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $^

tokenize.o: tokenize.c
	$(CC) $(CFLAGS) -c -o $@ $^

parse.o: parse.c
	$(CC) $(CFLAGS) -c -o $@ $^

codegen.o: codegen.c
	$(CC) $(CFLAGS) -c -o $@ $^
