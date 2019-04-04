all:	sorter_client sorter_server

sorter_client:	sorter_client.o
	gcc -o sorter_client sorter_client.o 

sorter_server: sorter_server.o
	gcc -o sorter_server sorter_server.o 

sorter_client.o: sorter_client.c
	gcc -c sorter_client.c

sorter_server.o: sorter_server.c
	gcc -c sorter_server.c
clean:	
	rm -f *.o sorter

