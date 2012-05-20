#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O2

all: rand-fork

rand-fork: pipesem.o rand-fork.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o rand-fork
