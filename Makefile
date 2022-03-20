#kindaOS
CC=gcc
CFLAGS=-O3 -std=c11 -g
PROJ=kernel
SRC=window.o kindaText.o taskbar.o main.c

default: $(PROJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

$(PROJ): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean: cleanO
	del *.exe
cleanO:
	del *.o