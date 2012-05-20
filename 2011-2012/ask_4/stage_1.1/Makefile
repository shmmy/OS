#
# Makefile
#
# Operating Systems, Exercise 4
#

CC = gcc
CFLAGS = -Wall -g
#CFLAGS = -Wall -O2 -g

.PHONY: all clean test

all: scheduler prog

scheduler: scheduler.o proc-common.o queue.o
	$(CC) -o scheduler queue.o scheduler.o proc-common.o
prog: prog.o proc-common.o
	$(CC) -o prog prog.o proc-common.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
clean:
	rm -f scheduler prog execve-example strace-test *.o
test:	all
	./scheduler prog prog prog 2>&1 | tee scheduler.log
