CC=gcc
CFLAGS=-Wpedantic -Wall -Wextra

shell: shell.c parse.c parse.h
	${CC} ${CFLAGS} -o shell shell.c parse.c

.PHONY: clean

clean:
	rm -f shell