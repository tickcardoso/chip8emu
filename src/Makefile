CC=gcc
CFLAGS=-std=c99 -Wall -pedantic-errors -g
LDFLAGS=-lSDL2 -lncurses

all: clean chip8.o main.o main

main: chip8.o main.o
	$(CC) -o main {main.o,chip8.o} $(CFLAGS) $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

chip8.o: chip8.c
	$(CC) -c chip8.c -o chip8.o $(CFLAGS)

clean:
	rm -f *.o
