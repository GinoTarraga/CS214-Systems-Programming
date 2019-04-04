#include "Sorter.h"
#include <dirent.h>
#include "mergesort.c"

/* prints an error message and exits the program */
void ERROR(char *error)
{
    fprintf(stderr,"ERROR: %s\n",error);
    exit(1);
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
    char *buffer;
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

void traverse(char* folder, char* sort,char* outputFolder)
{
	char out[1024];

	if(outputFolder != NULL){
		strcpy(out,outputFolder);
	}

	DIR *directory;
	directory = opendir(folder); // opens directory

	while(directory != NULL){
		struct dirent *entry = NULL;
		while(entry = readdir(directory)){ // traverses to following file
			printf("%s %s\n",folder,entry->d_name);
			if (entry->d_type == 8){ // checks if it is a file
				char *dot = strrchr(entry->d_name,'.');
				if (dot && !strcmp(dot,".csv") && !strstr(entry->d_name, "-sorted-")){ //checks if the file is a non sorted csv file, and then forks
					pid_t filePID = fork();
					if (filePID < 0 )
						printf("ERROR IN FILE\n");
					else if (filePID == 0){
						printf("found CSV: %s\n",entry->d_name);
						char filename[256];
						strcpy(filename,folder);
						strcat(filename,"/");
                				strcat(filename,entry->d_name);
						FILE *inCSV = fopen(filename, "r");
						printf("made File\n");
						CSV *csv = inputCSV(inCSV);
						printf("made CSV\n"); // this process opens the csv and makes it a CSV struct

						sortCSV(csv,sort); // merge sorts csv 
						close(inCSV);
						FILE *outCSV;

						if(outputFolder != NULL){
 // if there is a selected output folder, it makes the sorted CSVs there, if not then it makes it where the CSV is found
							char *name = strtok(entry->d_name, ".");
							strcat(out,"/");
							strcat(out,name);
							strcat(out, "-sorted-");
							strcat(out, sort);
							strcat(out, ".csv");
							printf("%s\n",out);
							outCSV = fopen(out, "w");
							outputCSV(csv,outCSV);
						} else {
							char *name = strtok(filename, ".");
							strcat(name, "-sorted-");
							strcat(name, sort);
							strcat(name, ".csv");
						
							outCSV = fopen(name, "w");
							outputCSV(csv,outCSV);
						}

						close(outCSV);
						deleteCSV(csv);
						printf("sorted and printed\n");
						exit(0);
					} else wait(NULL); // this is the parent fork, waits on child
				}
			}
			if(entry->d_name[0] != '.' && entry->d_type == 4){ // checks if the current selection is a directory, if so it forks.
				pid_t directoryPID = fork();
				if (directoryPID < 0)
					printf("ERROR IN DIRECTORY\n");
				else if (directoryPID == 0){ // child process takes this directory and sets it as the targeted directory to traverse next for CSV files.

					char next[256];
                			strcpy(next,folder);
                			strcat(next,"/");
               				strcat(next,entry->d_name);
					strcpy(folder,next);
					directory = opendir(folder);
				} else wait(NULL); // parent waits on child and then continues to the next entry in the original directory				
			} 
			
		}
		directory = NULL; // end of traversing entire directory tree and then exits.
		exit(0);
	}
						
}

int main(int argc,char **argv)
{
    char buffer[100];
    CSV *csv;
    char *sort;
    char *input;
    char *output = NULL;

	if(argc == 3){
		sort = argv[2];
		char cwd[1024];
		getcwd(cwd, sizeof (cwd));
		traverse(cwd, argv[2], output); 
	} else if (argc == 5){
		sort = argv[2];
		input = argv[4];
		traverse(input, argv[2], output); 
	} else if (argc == 7){
		sort = argv[2];
		input = argv[4];
		output = argv[6];
		traverse(input, argv[2], output); 
	} else{
		printf("INPUT ERROR. CLOSING PROGRAM\n");
		exit(-1);
	}
    
    
	sort = argv[2];
	input = argv[4];
	output = argv[6];
	printf("%d\n",argc);
	printf("%s\n",argv[1]);
	printf("%s\n" , sort);
	printf("%s\n" , input);
	printf("%s\n" , output);

    exit(0);
}
