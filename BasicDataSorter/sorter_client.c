#include "Sorter.h"

int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}


void *foundCSV(void *arg){

	INFO *in = (INFO *)arg;
	int sock = in->sock;
	int n;
	char request[4];
	strcpy(request,"sort");
	char sort[1024];
	bzero(sort,1024);
	strcpy(sort,in->sort);

	FILE *inCSV = fopen(in->currentFile, "r");
	char buffer[1024];
	if((n = strcmp(fgets(buffer,1024,inCSV),"color,director_name,num_critic_for_reviews,duration,director_facebook_likes,actor_3_facebook_likes,actor_2_name,actor_1_facebook_likes,gross,genres,actor_1_name,movie_title,num_voted_users,cast_total_facebook_likes,actor_3_name,facenumber_in_poster,plot_keywords,movie_imdb_link,num_user_for_reviews,language,country,content_rating,budget,title_year,actor_2_facebook_likes,imdb_score,aspect_ratio,movie_facebook_likes")) != 13){
		printf("Incorrect CSV Header!\n");
		return NULL;
	}
	rewind(inCSV);
	bzero(buffer,1024);

	if (inCSV){ // if so, continue sorting CSV, print to master file, and finish threading

		if(write(sock,request,5) < 0)
			return NULL;
		else
			usleep(10000);
		n = strlen(sort);

		if((n = write(sock,sort,n)) < 0)
			return NULL;
		else
			sleep(1);

		int i = 0;
		while(fgets(buffer,1024,inCSV)){
			//printf("%s",buffer);
			if(write(sock,buffer,strlen(buffer)) < 0 )
				return NULL;
			usleep(500);
			bzero(buffer, 1024);
			i++;
		}
		
		printf("Done\n");
		memset(buffer, 0, 1024);
		fclose(inCSV);
		bzero(buffer,1024);
		strcpy(buffer,"break");
		usleep(1000);
		n = write(sock,buffer,strlen(buffer));
		bzero(buffer,1024);
		read(sock,buffer,1024);
		printf("%s\n",buffer);
		return NULL;

	} else{                    //if not, then stop and stop thread
		printf("Incorrect CSV type.\n");
		fclose(inCSV);
		return NULL;
	}	
}

void *printSortedCSV(void *in)
{
	INFO *inInfo = (INFO*)in;
	//printf("Requesting Sorted CSV File: %s\n",inInfo->output);
	FILE *outCSV = inInfo->csvFile;
	
	rewind(outCSV);
	int sock = inInfo->sock;
	char request[4], server_message[1024];
	strcpy(request,"dump");
	write(sock,request,4);
	sleep(1);
	int i = 0, size;

	while((size = read(sock, server_message,1023)) > 0){
		if(strcmp(server_message,"Dumped") == 0)
			break;

		fprintf(outCSV,server_message);
		bzero(server_message,1024);

	}

	fclose(outCSV);
}


void *traverse(void *in)
{
	INFO *inInfo = (INFO*)in;

	pthread_t pth;

	DIR *directory;
	directory = opendir(inInfo->input); // opens start directory

	if(directory){
		struct dirent *entry = NULL;
		while((entry = readdir(directory)) != NULL){ // traverses to next entry
			if (entry->d_type == 8){ // checks if it is a file
				char *dot = strrchr(entry->d_name,'.');
				if (dot && !strcmp(dot,".csv") && !strstr(entry->d_name, "-sorted-") && strcmp(entry->d_name,"out.csv") && strcmp(entry->d_name,"tempCSV.csv")){ //checks if the file is a non sorted csv file, and then threads
					char filename[256];
					strcpy(filename,inInfo->input);
					strcat(filename,"/");
                			strcat(filename,entry->d_name);
					inInfo->currentFile = filename;
					printf("found CSV file %s\n",inInfo->currentFile);
					pthread_create(&pth, NULL, foundCSV, inInfo);
					pthread_join(pth,NULL);
				}
			} else if(entry->d_name[0] != '.' && entry->d_type == 4){ // checks if the current selection is a directory, if so it threads
					char next[1024];
					strcpy(next,"");
                			strcpy(next,inInfo->input);
                			strcat(next,"/");
               				strcat(next,entry->d_name);
									// makes a new INFO struct to not lose which directory you came from
						INFO *nextInfo = (INFO *)malloc(sizeof(INFO));
						nextInfo->sort =inInfo->sort;
						nextInfo->input = next;
						nextInfo->output = inInfo->output;
						nextInfo->csvFile = inInfo->csvFile;
						nextInfo->sock = inInfo->sock;

					pthread_create(&pth, NULL, traverse, nextInfo);
					pthread_join(pth,NULL);
					free(nextInfo);                  //frees the created INFO struct we created before						
			}
			
		}
	}					
}

int main(int argc, char *argv[]){

	char *sort;
	char *input;
	char *output = NULL;
	char *hostname;
	int port;
	char ip[100];

	if(argc == 7){
		if(strcmp(argv[1], "-c") == 0 && strcmp(argv[3],"-h") == 0 && strcmp(argv[5], "-p") == 0){
			sort = argv[2];
			hostname = argv[4];
			port = atoi(argv[6]);
			char cwd[1024];
			getcwd(cwd, sizeof (cwd));
			strcpy(input,cwd); 
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
	} else if (argc == 9){
		if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-h") == 0 && strcmp(argv[5], "-p") == 0 && strcmp(argv[7], "-d") == 0){
			sort = argv[2];
			hostname = argv[4];
			port = atoi(argv[6]);
			input = argv[8];
		} else if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-h") == 0 && strcmp(argv[5], "-p") == 0 && strcmp(argv[7], "-o") == 0){
			sort = argv[2];
			hostname = argv[4];
			port = atoi(argv[6]);
			output = argv[8];
			char cwd[1024];
			getcwd(cwd, sizeof (cwd));
			strcpy(input,cwd);
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
	} else if (argc == 11){
		if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-h") == 0 && strcmp(argv[5], "-p") == 0 && strcmp(argv[7], "-d") == 0 && strcmp(argv[9],"-o") == 0){
			sort = argv[2];
			hostname = argv[4];
			port = atoi(argv[6]);
			input = argv[8];
			output = argv[10];
		} else if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-h") == 0 && strcmp(argv[5], "-p") == 0 && strcmp(argv[7], "-o") == 0 && strcmp(argv[9],"-d") == 0){
			sort = argv[2];
			hostname = argv[4];
			port = atoi(argv[6]);
			input = argv[10];
			output = argv[8];
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
		
	} else{
		printf("INPUT ERROR. CLOSING PROGRAM\n");
		exit(-1);
	}

	hostname_to_ip(hostname , ip);

	int clientSocket;
  	char buffer[1024];
  	struct sockaddr_in serverAddr;
  	socklen_t addr_size;

  	/*---- Create the socket. The three arguments are: ----*/
  	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
  	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  	/*---- Configure settings of the server address struct ----*/
  	/* Address family = Internet */
  	serverAddr.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	serverAddr.sin_port = htons(port);
  	/* Set IP address to localhost */
  	serverAddr.sin_addr.s_addr = inet_addr(ip);
  	/* Set all bits of the padding field to 0 */
  	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  	/*---- Connect the socket to the server using the address struct ----*/
  	addr_size = sizeof serverAddr;
  	connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);  


	INFO *start = (INFO *)malloc(sizeof(INFO));
	start->sort = sort;
	start->input = input;
	start->output = output;
	start->sock = clientSocket;

	char filename[1024] = "AllFiles-sorted-";
	strcat(filename, start->sort);
	strcat(filename, ".csv");

	if(start->output != NULL){
		char f[1024] = "";
		strcpy(f, start->output);
		strcat(f,"/");
		strcat(f,filename);
		strcpy(filename,f);
	} else{
		char f[1024] = "";
		strcpy(f, start->input);
		strcat(f,"/");
		strcat(f,filename);
		strcpy(filename,f);
	}
	

	start->csvFile = fopen(filename, "w");
	if(start->csvFile != NULL)
		printf("Opened output file %s\n",filename);
	else{
		printf("Failed opening outputfile %s\n",filename);
		exit(-1);
	}

	traverse(start);
	printSortedCSV(start);
	shutdown(clientSocket,0);
	printf("All finished\n");
	exit(0);
}
