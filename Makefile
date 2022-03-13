CC=gcc
CFLAGS=-O3 -std=c11 -g
SRC=main.c window.c


default: main

main: $(SRC)
	$(CC) $(CFLAGS) $^ -o kernel