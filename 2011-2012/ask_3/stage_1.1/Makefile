#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O2

all: pipesem.o pipesem-test

pipesem-test: pipesem.o proc-common.o pipesem-test.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o pipesem-test

