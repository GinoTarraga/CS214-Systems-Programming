#include "Sorter.h"
#include "mergesort.c"

/* global variable to keep track of IP addresses */
List *IPList;

/* global variable to keep track of process ID */
int processID;

List *newList()
{
        List *list = malloc(sizeof(List));
        if (list == NULL)
        {
                printf("Unable to allocate memory for list\n");
                exit(1);
        }
        // list->size=0;
        list->head=NULL;
        return list;
}

void insert(void *data, List *list)
{
        Node *tmp, *node;

        node = malloc(sizeof(Node));
        if (node == NULL)
        {
                printf("Unable to allocate memory for node\n");
                exit(1);
        }
        node->data = data;
        node->next=NULL;

        pthread_mutex_lock(&list->mutex);
        tmp = list->head;
        list->head = node;
        list->head->next = tmp;

        // list->size++;
        pthread_mutex_unlock(&list->mutex);
}

void destroyList(List *list)
{
        Node *node, *tmp;
        node = list->head;
        while(node)
        {
                tmp=node;
                node = node->next;
                if ((tmp->data) != NULL)
                {
                        free(tmp->data);
                }
                free(tmp);
        }
        free(list);
}

/* Handler for SIGTERM, the default signal sent by SIGKILL to terminate program */
void handler(int sig)
{
	if ((sig==SIGINT) ||(sig==SIGTERM) || (sig== SIGALRM))
	{
        	// printf("handler\n");
        	Node *node, *tmp;
       		char str[1024];
        	printf("Recieved connections from:");

		if (IPList == NULL)
		{
			printf("Exited prematurely\n");
			exit(1);
		}

        	node = IPList->head;
        	// printf("addr of head: %p\n", node);

        	while(node)
        	{
               	 	tmp=node;
                	inet_ntop(AF_INET, tmp, str, 1024);
                	printf("%s",str);
                	if((node->next) != NULL)
                       		printf(",");
                	printf(" ");
                	node = node->next;
        	}

        	destroyList(IPList);
		if (sig == SIGALRM)
		{
			printf("Exited on some kind of error\n");
        		exit(1);
		}
		printf("\nExited successfully\n");
		exit(0);
	}
	else
	{
		printf("\nthis should not have happened\n");
		exit(1);
	}
}



/* prints an error message and exits the program */
void ERROR(char *error)
{
    printf("ERROR: %s\n",error);
    /* Use SIGALRM for error handling */
    if (signal(SIGALRM, handler) == SIG_ERR)
    {
	printf("Could not install SIGALRM handler\n");
	exit(1);
    }

    kill(processID, SIGALRM);
    //exit(1);
}

/* wrapper around malloc to detect out of memory errors */
void *safeMalloc(size_t size)
{
    void *p;
    p=malloc(size);
    if(p==NULL)
        ERROR("Out of memory!");
    return p;
}

/* wrapper around realloc to detect out of memory errors */
void *safeRealloc(void *ptr,size_t size)
{
    void *p;
    p=realloc(ptr,size);
    if(p==NULL)
        ERROR("Out of memory!");
    return p;
}

/* count the number of comma separated columns in the given line */
int countColumns(char *line)
{
    char *p;
    int n;
    
    n=1;    /* by default, line contains a field */
    p=line;
    while(*p)
    {
        if(*p==',') /* we found a comma */        
            n++; /*increment number of fields*/
        p++;
    }
    return n;   /* return the number of fields */
}

/* split a given line using the commas as delimiters, returns an array of
data, expects n columns of data*/
Data *commaSplit(char *line,int n)
{
    int start;
    int  pos,col;
    Data *parts;
    int inquotes;
    
    parts=(Data *)safeMalloc(n*sizeof(Data)); /* allocate space for columns*/
    pos=0;
    start=0; /*start of current column data*/
    col=0;  /*current column number */
    inquotes=0;
    while(line[pos]!=0 && line[pos]!='\r' && line[pos]!='\n')
    {
        if(line[pos] == '\"')
            inquotes=!inquotes;
        if(!inquotes && line[pos]==',') /* we found a comma */        
        {
            parts[col].s=(char *)safeMalloc(pos-start+1); /* allocate space for column data*/
            strncpy(parts[col].s,&line[start],pos-start);
            parts[col].s[pos-start]=0; /* insert string terminator */
            start=pos+1;  /* column data starts after the comma*/
            col++;  /*increment current column*/
        }
        pos++;
    }
    parts[col].s=(char *)safeMalloc(pos-start+1); /* allocate space for last column*/
    strncpy(parts[col].s,&line[start],pos-start);
    parts[col].s[pos-start]=0; /* insert string terminator */
    return parts;
}

/* inserts a new row in the csv structure */
void insertRow(Data *rowdata,CSV *csv)
{
    //int col;
    
    if(csv->nrows==0)
        csv->rows=(Data **)safeMalloc(sizeof(Data *));  /*create single row entry */
    else       
        csv->rows=(Data **)safeRealloc(csv->rows,(csv->nrows+1)*sizeof(Data *));  /*create single row entry */
    csv->rows[csv->nrows]=rowdata;
    csv->nrows++;
}

/* creates a new CSV structure */
CSV *newCSV(int nrows)
{
    CSV *csv;
    csv=(CSV*)safeMalloc(sizeof(CSV));  /* create new CSV structure */
    csv->ncols=0;
    csv->nrows=nrows;
    csv->header=NULL;
    if(nrows==0)
        csv->rows=NULL;
    else
        csv->rows=(Data **)safeMalloc(csv->nrows*sizeof(Data *));  /*create rows */        
    csv->types=NULL;
    return csv;
}

/* frees all the space used by the csv structure*/
void deleteCSV(CSV *csv)
{
    int i,j;
    if(csv->header)
    {
        for(j=0; j<csv->ncols; j++)
            free(csv->header[j].s);
        free(csv->header);
    }
    for(i=0; i<csv->nrows; i++)
    {
        for(j=0; j<csv->ncols; j++)
            if(csv->types[j]==STRING)
                free(csv->rows[i][j].s);
        free(csv->rows[i]);
    }
    if(csv->types)
        free(csv->types);
    free(csv->rows);
    free(csv);
 
}

/* returns 1 if the string is an integer, 0 if it is not, -1 if an error is found
*/
int isInteger(char *val) 
{
    int i;

    if(val[0]==0)
        return -1;
    i=0;
    if(val[i]=='+' || val[i]=='-')  /* skip sign if found */
        i++;
    for(; i<strlen(val); i++)
        if(!isdigit(val[i]))
            return 0;
    return 1;
}

/* returns 1 if the string is a float, 0 if it is not, -1 if an error is found
*/
int isFloat(char *val) 
{
    int i;

    if(val[0]==0)
        return -1;
    i=0;
    if(val[i]=='+' || val[i]=='-')  /* skip sign if found */
        i++;
    for(; i<strlen(val); i++)
        if(!isdigit(val[i]))
            break;
    if(val[i++]!='.')
        return 0;
    for(; i<strlen(val); i++)
        if(!isdigit(val[i]))
            return 0;
    return 1;
}

/* detect the type of column in the csv */
int detectColumnType(int col,CSV *csv)
{
    int isi,isf;
    int row;
    int type;
    
    type=UNKNOWN;
    for(row=0; row<csv->nrows; row++)
    {
        if((isf=isFloat(csv->rows[row][col].s))==1)
        {
            if(type!=STRING)
                type=FLOAT;
        }
        else if((isi=isInteger(csv->rows[row][col].s))==1)
        {
            if(type!=STRING && type!=FLOAT)
                type=INTEGER;
        }
        else if(!isi && !isf)
            type=STRING;        
    }
    if(type==UNKNOWN)
        type=STRING;
    return type;
}   

/* set the types for all columns in the CSV*/
void setTypes(CSV *csv)
{

    int row,col;
    char *p;
    
    csv->types=(int*)safeMalloc(csv->ncols*sizeof(int));
    for(col=0; col<csv->ncols; col++)
        csv->types[col]=detectColumnType(col,csv);   
    for(col=0; col<csv->ncols; col++)
    {
        if(csv->types[col]==INTEGER)
        {
            for(row=0; row<csv->nrows; row++)
            {
                p=csv->rows[row][col].s;
                csv->rows[row][col].i=atoi(p);
                free(p);
            }
        }
        else if(csv->types[col]==FLOAT)
        {
            for(row=0; row<csv->nrows; row++)
            {
                p=csv->rows[row][col].s;
                csv->rows[row][col].f=atof(p);
                free(p);
            }
        }
    }

}

/* parses a CSV file given in stdin and saves it in a CSV structure  */
CSV *inputCSV(FILE *inputFile)
{
    CSV *csv;
    char buffer[1024];
    Data *rowdata;

    if(!fgets(buffer,1024,inputFile))   /* read header */
        ERROR("Empty file.");

    csv=newCSV(0);
    csv->ncols=countColumns(buffer);       /* get expected number of columns */
    csv->header=commaSplit(buffer,csv->ncols);   /* fill column names */
    while(fgets(buffer,1024,inputFile) != NULL)   /* read line by line*/
    {
        rowdata=commaSplit(buffer,csv->ncols);   /* get column values for current row */
        insertRow(rowdata,csv);    /* insert a new row in the csv structure */
	bzero(buffer,1024);
    }
    setTypes(csv);

    return csv;
}

/* send CSV structure to stdout */
void outputCSV(CSV *csv, FILE *outputFILE )
{
    int i,j;
        
    for(i=0; i<csv->ncols; i++)
    {
        fprintf(outputFILE, "%s",csv->header[i].s);
        if(i<csv->ncols-1)
            fprintf(outputFILE, ",");
    }
    fprintf(outputFILE, "\n");
    for(i=0; i<csv->nrows; i++)
    {
        for(j=0; j<csv->ncols; j++)
        {
            if(csv->types[j]==STRING)
                fprintf(outputFILE,"%s",csv->rows[i][j].s);
            else if(csv->types[j]==INTEGER)
                fprintf(outputFILE,"%d",csv->rows[i][j].i);
            if(csv->types[j]==FLOAT)
                fprintf(outputFILE,"%lf",csv->rows[i][j].f);
            if(j<csv->ncols-1)
                fprintf(outputFILE,",");
        }
        fprintf(outputFILE,"\n");
    }  
    fprintf(outputFILE,"\n");
 
}

/* sorts the csv using the selected column*/
void sortCSV(CSV *csv,char *column)
{
    int col;
    for(col=0; col<csv->ncols; col++)
        if(!strcmp(csv->header[col].s,column))
            break;
    if(col==csv->ncols)
    {
        deleteCSV(csv);       
        ERROR("Column was not found!");
	exit(-1);
    }
    mergeSort(csv,col);
}

/* compare two strings using the type to convert them to an integer 
or a float*/
int compare(Data a,Data b,int type)
{
    switch(type)
    {
        case STRING:    /* if they are strings */
            return strcmp(a.s,b.s);
            break;
        case INTEGER:   /* if they are integers, */
            if(a.i>b.i)
                return 1;
            else if(a.i<b.i)
                return -1;
            else
                return 0;
            break;
        case FLOAT: /* if the values are floating point */
            if(a.f>b.f)
                return 1;
            else if(a.f<b.f)
                return -1;
            else
                return 0;
            break;
    }
    return 0;    
}

void *request(void *arg){

	int sock = *(int*)arg;
	int buff_size;
	char column[1024];
	FILE *outCSV = fopen("out.csv","w");
	char message[1024], client_message[1024], filename[1024];
	//sleep(2);
	printf("called request\n");
	while(read(sock, client_message, 1023) > 0){

		if(strcmp(client_message,"sort") == 0){

			bzero(client_message,1024);
			read(sock,client_message,1023);
			strcpy(column, client_message);
			bzero(client_message,1024);


			FILE *tempCSV = fopen("tempCSV.csv", "w");
			int size;
			int i = 0;
			while((size = read(sock, client_message,1023)) > 0){

				if(strcmp(client_message,"break") != 0){
					fprintf(tempCSV,client_message);
					bzero(client_message,1024);
					i++;
					//printf("line: %d\n",i);
				} else
					break;
			}

			bzero(client_message,1024);
			fclose(tempCSV);
			FILE *newCSV = fopen("tempCSV.csv", "r");

			rewind(tempCSV);
			CSV *csv = inputCSV(newCSV);

			fclose(newCSV);
			sortCSV(csv,column);
			outputCSV(csv, outCSV);
			deleteCSV(csv);
			remove("tempCSV.csv");
			send(sock,"Sorted",strlen("Sorted"),0);
			usleep(1000);

		} else if(strcmp(client_message, "dump") == 0){

			fclose(outCSV);
			outCSV = fopen("out.csv", "r");
			rewind(outCSV);
			sleep(1);
			int c = 0;
			bzero(message,1024);
			while(fgets(message,1024,outCSV)){
				send(sock,message,strlen(message),0);
				usleep(100);
				bzero(message,1024);
				c++;
			}
			sleep(1);

			fclose(outCSV);
			remove("out.csv");
			send(sock,"Dumped",strlen("Dumped"),0);
			break;
		}
	}
}

int main(int argc,char **argv){
        processID = getpid();
	char USAGE[] = "USAGE: ./sorter_server -p port";
	if(strcmp(argv[1],"-p") != 0 || argc < 3){
		printf("Input Error, Need Port\n");
		printf("%s\n",USAGE);
		exit(1);
	}
	if (argc > 3)
	{
		printf("too many parameters\n");
		printf("%s\n",USAGE);
		exit(1);
	}
	

	char ip_address[15];
    	int fd;
	int count = 0;
    	struct ifreq ifr;
    	fd = socket(AF_INET, SOCK_DGRAM, 0);
    	ifr.ifr_addr.sa_family = AF_INET;
    	memcpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    	ioctl(fd, SIOCGIFADDR, &ifr);
    	close(fd);
    	strcpy(ip_address,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
     
  	int welcomeSocket, newSocket;
  	char buffer[1024];
  	struct sockaddr_in serverAddr, clientAddr;
	struct in_addr *ipAddrPtr;
  	// struct sockaddr_storage serverStorage;
  	socklen_t addr_size;


        /* install handler to deal with SIGTERM and SIGINT */
        if (signal(SIGTERM, handler) == SIG_ERR)
                ERROR("Could not install SIGTERM handler");
        if (signal(SIGINT, handler) == SIG_ERR)
                ERROR("Could not install SIGINT handler");
        printf("Process ID of server: %d\n", processID);


	/*
  	welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (welcomeSocket < 0)
		ERROR("Could not open socket");
  
        bzero((char *)&serverAddr, sizeof(serverAddr));
	*/

        IPList=newList();

  	/*---- Configure settings of the server address struct ----*/
  	serverAddr.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	serverAddr.sin_port = htons(atoi(argv[2]));

  	serverAddr.sin_addr.s_addr = INADDR_ANY;
  	/* Set all bits of the padding field to 0 */
  	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  	/*---- Bind the address struct to the socket ----*/
	/*
  	if((bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0)
		ERROR("Couldn't bind socket");
	
	*/
  	/*---- Listen on the socket, with 5 max connection requests queued ----*/
	/*
  	if(listen(welcomeSocket,5)==0)
  	  	printf("Listening\n");
  	else
  	  	printf("Error\n");
	*/

  	/*---- Accept call creates a new socket for the incoming connection ----*/
  	// addr_size = sizeof serverStorage;
  	addr_size = sizeof(clientAddr);
  	//newSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
	
  	/*---- Send message to the socket of the incoming connection ----*/
  	//strcpy(buffer,"Hello World\n");
  	//send(newSocket,buffer,13,0);

        welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (welcomeSocket < 0)
	        ERROR("Could not open socket");
        if((bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0)
                ERROR("Couldn't bind socket");

        if(listen(welcomeSocket,5)==0)
                printf("Listening\n");
        else
                printf("Error\n");


                newSocket = accept(welcomeSocket, (struct sockaddr *) &clientAddr, &addr_size);
                printf("new listening socket created\n");

                if(newSocket < 0)
                        ERROR("Error on accepting");

	while(1 == 1){

		// newSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
		
		/*
	        welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
        	if (welcomeSocket < 0)
               		ERROR("Could not open socket");

	      	if((bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0)
                	ERROR("Couldn't bind socket");

	     	 if(listen(welcomeSocket,5)==0)
                	printf("Listening\n");
              	else
               		printf("Error\n");

		
		newSocket = accept(welcomeSocket, (struct sockaddr *) &clientAddr, &addr_size);
		printf("new listening socket created\n");

		if(newSocket < 0)
			ERROR("Error on accepting"); 
		*/
		ipAddrPtr = malloc(sizeof(struct in_addr));
                if (ipAddrPtr==NULL)
                {
                        printf("error allocating memory for ipAddrPtr\n");
                        exit(1);
                }
                *ipAddrPtr = clientAddr.sin_addr;
                insert(ipAddrPtr, IPList);

                if (ipAddrPtr==NULL)
                {
                        printf("error allocating memory for ipAddrPtr\n");
                        exit(1);
                }

		// printf("Recieved Connection %d on Socket %d\n",newSocket);
		// printf("Recieved Connection %d on Socket %d\n",count, newSocket);
		bzero(buffer,1024);
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, request, (void*)&newSocket);
		pthread_join(thread_id,NULL);
		shutdown(newSocket,0);
		printf("Waiting for next connection.\n");

	}

  	return 0;
}
