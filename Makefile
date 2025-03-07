CFLAGS = -Wall -g -pthread -o server
PROGS = mos main
PROGRAM_NAME = server
CC = gcc-14

all: $(PROGS)

main: main.c
	$(CC) $(CFLAGS) -o $@ $^

mos: mos.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o
	rm -f $(PROGS)

install:
	cp $(PROGS) $(TARGETDIR)/bin

run:
	./main