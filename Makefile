CC     := gcc
CFLAGS := -std=c17
CFLAGS += -pedantic-errors
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wno-missing-field-initializers
CFLAGS += -O2

.PHONY: all
all: main

.PHONY: test
test: main test.sh Makefile
	-bash test.sh

.PHONY: clean
clean:
	-rm -f main codegen.o main.o parse.o string.o tokenize.o type.o
	-rm -f tmp tmp.s sub.o

main: codegen.o main.o parse.o string.o tokenize.o type.o Makefile
	$(CC) -o $@ $(filter-out Makefile, $^)

codegen.o: codegen.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

parse.o: parse.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

string.o: string.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

tokenize.o: tokenize.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

type.o: type.c main.h Makefile
	$(CC) $(CFLAGS) -c -o $@ $<
