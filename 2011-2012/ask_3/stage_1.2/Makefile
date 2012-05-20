#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O2

all: mandel 

mandel: mandel-lib.o mandel.o pipesem.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o mandel
