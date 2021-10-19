CC=gcc
CFLAGS=-std=c99 -Wall -pedantic-errors
LDFLAGS=-lSDL2 -lncurses

all: clean chip8.o main.o main

main: chip8.o main.o chip8_dbg.o
	$(CC) -o chip8emu main.o chip8.o chip8_dbg.o $(CFLAGS) $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

chip8_dbg.o: chip8_dbg.c
	$(CC) -c chip8_dbg.c $(CFLAGS)

chip8.o: chip8.c
	$(CC) -c chip8.c $(CFLAGS)

clean:
	rm -f *.o
