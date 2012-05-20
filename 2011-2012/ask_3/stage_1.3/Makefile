#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O2

all: procs-shm pipesem.o

procs-shm: proc-common.o procs-shm.o pipesem.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o procs-shm
