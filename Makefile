all: dsh.o 
	gcc -Wall -g -o dsh main.c dsh.o

dsh.o: dsh.h dsh.c
	gcc -Wall -g -c dsh.c

clean:
	rm -f dsh *.o
