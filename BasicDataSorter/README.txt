README

/**************************************************************************************
Goal of each program

The program Sorter_client.c traverses all subdirectories under the directory which is inputted by the user. A new thread is spawned when the file is found, which then creates a new search request. It then connects to the server, the client then waits for the request to be processed which then reads back the respond. After all the files under the search directory are send to the server, another request is send to give it the sorted versions of the files that were previously sent. The server which is sorter_server.c then sends back all the sorted versions of all the files. The client takes a long time to search/send due to usleep logic which the server halts if nonMovie csv data reads in. When the client connects to the server. The client then outputs a single csv. 

The program sorter_server.c opens the request that were sent by the client. Every request, the server will spawn a new thread to handle the connection which waits for future connections. Every service thread checks if its a sort request. If true, it performs the sort and stores the data inside the server. If not true, then it must be a dump request, which then merges the current sorted data into one sorted csv which is sent back to the client. The server runs until it is killed by a sigkill. Because of the logic used to avoid synchronization errors, the program may take a long time to finish. Please wait until no the server indicates that all connections have been satisfied before killing the program.

**************************************************************************************/

/**************************************************************************************
ASSUMPTIONS- how our program actually works

1. We used the TCP/IP protocol for network and socket connections.
2. The server exits if it encounters a .CSV file that doesn't contain movie data.
3. We thread only once on the server and have the server read in data sent from the
user in only one connecting socket. As a result, we have that the server will terminate correctly only when it begins requesting for more threads.

4. Client does thread multiple times correctly upon traversing and checking CSV data.
5. Server usage: ./sorter_server -p port
6. Client usage: ./sorter_client -c colName -h hostName -p port <-d directory_to_begin_search> <-o outputDirectory>
7. We expect these parameters to be entered in this order, if they exist.
8. The single file combining all movie data has the name
"AllFiles-sorted-[column sorted by].csv" and is output to the output directory
9. If not provided, client searches at current directory and/or outputs combined file in current directory.
10.To run, call make, then sorter_server (with the correct parameters), then sorter_client (with the correct parameters).
**************************************************************************************/

/**************************************************************************************
Organization of program

1. "Sorter.h" contains all global variable definitions and typedefs used. It also 
includes the function prototypes of those that have not been altered greatly from previous projects.
It does not include the prototypes of functions introduced in this project. These functions may be found in their respective sorter_client and sorter_server .c files.
2.  Makefile = makefile used to compile the executables sorter_client.exe and sorter_server.exe 
**************************************************************************************/
