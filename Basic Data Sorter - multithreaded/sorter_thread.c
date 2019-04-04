#include "Sorter.h"
#include <dirent.h>
#include "mergesort.c"
#include <pthread.h>

/* prints an error message and exits the program */
void ERROR(char *error)
{
    printf("ERROR: %s\n",error);
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
    while(fgets(buffer,1024,inputFile))   /* read line by line*/
    {
        rowdata=commaSplit(buffer,csv->ncols);   /* get column values for current row */
        insertRow(rowdata,csv);    /* insert a new row in the csv structure */
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

void *foundCSV(void *arg){

	INFO *in = (INFO *)arg;

	FILE *inCSV = fopen(in->currentFile, "r");
	char buffer[1024];
	fgets(buffer,1024,inCSV);
	char *buf = strtok(buffer, "\n");
	
	char buff[1024];                 // compares the header line to the correct CSV format
	strcpy(buff,"color,director_name,num_critic_for_reviews,duration,director_facebook_likes,actor_3_facebook_likes,actor_2_name,actor_1_facebook_likes,gross,genres,actor_1_name,movie_title,num_voted_users,cast_total_facebook_likes,actor_3_name,facenumber_in_poster,plot_keywords,movie_imdb_link,num_user_for_reviews,language,country,content_rating,budget,title_year,actor_2_facebook_likes,imdb_score,aspect_ratio,movie_facebook_likes" );


	if (strcmp(buf,buff) == 13){ // if so, continue sorting CSV, print to master file, and finish threading
		rewind(inCSV);
		CSV *csv = inputCSV(inCSV);
		close(inCSV);
		sortCSV(csv,in->sort);
		outputCSV(csv,in->csvFile);
		printf("sorted\n");
		return;

	} else{                    //if not, then stop and stop thread
		printf("Incorrect CSV type.\n");
		close(inCSV);
		return;
	}

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
			//printf("%s   %s\n",inInfo->input,entry->d_name);               //Uncomment this if you want to see how it traverses the directories!!
			if (entry->d_type == 8){ // checks if it is a file
				char *dot = strrchr(entry->d_name,'.');
				if (dot && !strcmp(dot,".csv") && !strstr(entry->d_name, "-sorted-")){ //checks if the file is a non sorted csv file, and then threads
					char filename[256];
					strcpy(filename,inInfo->input);
					strcat(filename,"/");
                			strcat(filename,entry->d_name);
					inInfo->currentFile = filename;
					printf("found CSV file %s\n",inInfo->currentFile);
					pthread_create(&pth, NULL, foundCSV, inInfo);
					pthread_join(pth,NULL);
					printf("next entry\n");
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

					pthread_create(&pth, NULL, traverse, nextInfo);
					pthread_join(pth,NULL);
					free(nextInfo);                  //frees the created INFO struct we created before						
			}
			
		}
	}
						
}

int main(int argc,char **argv)
{
	char *sort;
	char *input;
	char *output = NULL;

	if(argc == 3){
		if(strcmp(argv[1], "-c") == 0){
			sort = argv[2];
			char cwd[1024];
			getcwd(cwd, sizeof (cwd));
			strcpy(input,cwd); 
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
	} else if (argc == 5){
		if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-d") == 0){
			sort = argv[2];
			input = argv[4];
		} else if(strcmp(argv[3],"-c") == 0 && strcmp(argv[1],"-d") == 0){
			sort = argv[4];
			input = argv[2];
		} else if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-o") == 0){
			sort = argv[2];
			output = argv[4];
			char cwd[1024];
			getcwd(cwd, sizeof (cwd));
			strcpy(input,cwd);
		} else if(strcmp(argv[3],"-c") == 0 && strcmp(argv[1],"-o") == 0){
			sort = argv[4];
			output = argv[2];
			char cwd[1024];
			getcwd(cwd, sizeof (cwd));
			strcpy(input,cwd);
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
	} else if (argc == 7){
		if(strcmp(argv[1],"-c") == 0 && strcmp(argv[3],"-d") == 0 && strcmp(argv[5],"-o") == 0){
			sort = argv[2];
			input = argv[4];
			output = argv[6];
		} else if(strcmp(argv[1],"-c") == 0 && strcmp(argv[5],"-d") == 0 && strcmp(argv[3],"-o") == 0){
			sort = argv[2];
			input = argv[6];
			output = argv[4];
		} else if(strcmp(argv[3],"-c") == 0 && strcmp(argv[1],"-d") == 0 && strcmp(argv[5],"-o") == 0){
			sort = argv[4];
			input = argv[2];
			output = argv[6];
		} else if(strcmp(argv[5],"-c") == 0 && strcmp(argv[1],"-d") == 0 && strcmp(argv[3],"-o") == 0){
			sort = argv[6];
			input = argv[2];
			output = argv[4];
		} else if(strcmp(argv[5],"-c") == 0 && strcmp(argv[3],"-d") == 0 && strcmp(argv[1],"-o") == 0){
			sort = argv[6];
			input = argv[4];
			output = argv[2];
		} else if(strcmp(argv[3],"-c") == 0 && strcmp(argv[5],"-d") == 0 && strcmp(argv[1],"-o") == 0){
			sort = argv[4];
			input = argv[6];
			output = argv[2];
		} else{
			printf("INPUT ERROR. CLOSING PROGRAM\n");
			exit(-1);
		}
		
	} else{
		printf("INPUT ERROR. CLOSING PROGRAM\n");
		exit(-1);
	}

	INFO *start = (INFO *)malloc(sizeof(INFO));
	start->sort = sort;
	start->input = input;
	start->output = output;

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
	printf("All finished\n");
	close(start->csvFile);

    exit(0);
}
