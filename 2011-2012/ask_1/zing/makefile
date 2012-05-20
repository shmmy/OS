all:	zing	
zing:   main.o
	gcc main.o zing.o -o zing -Wall -m32
main.o: main.c
	gcc -c main.c -o main.o -Wall -m32
clean:
	rm main.o zing
