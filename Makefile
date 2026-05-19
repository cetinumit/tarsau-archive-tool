all: derle bagla

derle:
	gcc -c main.c -o main.o
	gcc -c archive.c -o archive.o
	gcc -c extract.c -o extract.o

bagla:
	gcc main.o archive.o extract.o -o tarsau

clean:
	rm *.o tarsau