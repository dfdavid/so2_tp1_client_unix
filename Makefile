CC=gcc
CFLAGS= -Werror -Wall -pedantic -Wextra  -O1

all: tp1_client_u

tp1_client_u: main.o
	$(CC) -o tp1_client_u main.o

main.o: main.c
	cppcheck --enable=all --inconclusive --std=posix main.c
	$(CC) $(CFLAGS)  main.c -c

clean:
	rm -f tp1_client_u *.o
