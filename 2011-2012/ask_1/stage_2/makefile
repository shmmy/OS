all: 		fconc
fconc:		fconc.o
	gcc fconc.o -o fconc -m32
fconc.o: 	fconc.c fconc.h
	gcc -c fconc.c -o fconc.o -Wall -m32
.PHONY: clean test
clean:
	rm fconc.o fconc A B C D E F
test:
	echo "This is file A" > A
	echo "This is file B" > B
	echo "Right guess, file C"  > C
	echo "Yep, that's file D" > D
	echo "And that's file E" > E
	./fconc A B C D E A F
	cat F
strace:
	strace -o strace_outfile ./fconc A B C D E F

