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
	-rm -f main main.o
	-rm -f tmp tmp.s

main: main.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $^
