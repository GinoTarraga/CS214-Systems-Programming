#include "Sorter.h"

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
    int col;
    
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
CSV *inputCSV()
{
    CSV *csv;
    char buffer[1024];
    Data *rowdata;
    
    if(!fgets(buffer,1024,stdin))   /* read header */
        ERROR("Empty file.");
    csv=newCSV(0);
    csv->ncols=countColumns(buffer);       /* get expected number of columns */
    csv->header=commaSplit(buffer,csv->ncols);   /* fill column names */
    while(fgets(buffer,1024,stdin))   /* read line by line*/
    {
        rowdata=commaSplit(buffer,csv->ncols);   /* get column values for current row */
        insertRow(rowdata,csv);    /* insert a new row in the csv structure */
    }
    setTypes(csv);
    return csv;
}

/* send CSV structure to stdout */
void outputCSV(CSV *csv)
{
    int i,j;
        
    for(i=0; i<csv->ncols; i++)
    {
        printf("%s",csv->header[i].s);
        if(i<csv->ncols-1)
            printf(",");
    }
    printf("\n");
    for(i=0; i<csv->nrows; i++)
    {
        for(j=0; j<csv->ncols; j++)
        {
            if(csv->types[j]==STRING)
                printf("%s",csv->rows[i][j].s);
            else if(csv->types[j]==INTEGER)
                printf("%d",csv->rows[i][j].i);
            if(csv->types[j]==FLOAT)
                printf("%lf",csv->rows[i][j].f);
            if(j<csv->ncols-1)
                printf(",");
        }
        printf("\n");
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

int main(int argc,char **argv)
{
    char buffer[100];
    CSV *csv;
    
    if(argc<2)
        ERROR("Missing arguments.\nUSAGE:\n  ./sorter -c <column>");
    else if(strcmp(argv[1],"-c"))       
    {
        sprintf(buffer,"Invalid option \"%s\".",argv[1]);
        ERROR(buffer);
    }
    else if(argc==2)
        ERROR("Missing column name.\nUSAGE:\n  ./sorter -c <column>");    
    csv=inputCSV();   
    sortCSV(csv,argv[2]);
    outputCSV(csv);
    deleteCSV(csv);
    return 0;
}
