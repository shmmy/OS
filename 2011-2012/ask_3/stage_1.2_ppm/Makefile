#
# Makefile
#

CC = gcc
CFLAGS = -Wall -O3
LDFLAGS = -lm

all: mandel 

mandel: mandel-lib.o mandel.o pipesem.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<
resize:
	convert mandel.ppm mandel.png

clean:
	rm -f *.o mandel
isles:
	./runem.sh
merge:
	./merge.sh
