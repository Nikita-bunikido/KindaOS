#kindaOS
CC=gcc
CFLAGS=-O3 -std=c11 -g
PROJ=kernel
SRC=window.o kindaText.o taskbar.o main.c
HDR=taskbar.h window.h
HDRG=header.h

default: $(PROJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

$(PROJ): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $^

clean: cleanO
	del *.exe
cleanO:
	del *.o