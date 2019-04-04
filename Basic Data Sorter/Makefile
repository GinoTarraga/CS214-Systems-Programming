all:	sorter

sorter:	sorter.o mergesort.o
	gcc -o sorter sorter.o mergesort.o

sorter.o:	sorter.c
	gcc -c sorter.c

mergesort.o:	mergesort.c
	gcc -c mergesort.c

clean:	
	rm -f *.o sorter

