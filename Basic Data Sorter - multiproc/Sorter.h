#ifndef SORTER_H
#define SORTER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

/* detected data types */
#define UNKNOWN -1
#define STRING  0
#define INTEGER 1
#define FLOAT   2

/* definition of a single data point only an element is active for a given cell*/
typedef union
{
    char *s;    /* string data */
    int   i;    /* integer data */
    double f;   /* float data */
}Data;

/* definition of the csv structure */
typedef struct
{
    Data  *header;  /* contains the names of the columns */
    int    *types;  /* type of each column */
    Data   **rows;  /* rows of data */    
    int     nrows;  /* csv table dimensions */
    int     ncols;
}CSV;

typedef struct
{
	char *sort;
	char *input;
	char *output;
	char *currentFile;
	FILE *csvFile;
}INFO;

/* prints an error message and exits the program */
void ERROR(char *error);

/* wrapper around malloc to detect out of memory errors */
void *safeMalloc(size_t size);

/* wrapper around realloc to detect out of memory errors */
void *safeRealloc(void *ptr,size_t size);

/* count the number of comma separated columns in the given line */
int countColumns(char *line);

/* split a given line using the commas as delimiters, returns an array of
strings, expects n columns of data*/
Data *commaSplit(char *line,int n);

/* inserts a new row in the csv structure */
void insertRow(Data *rowdata,CSV *csv);

/* creates a new CSV structure */
CSV *newCSV(int nrows);

/* frees all the space used by the csv structure*/
void deleteCSV(CSV *csv);

/* returns 1 if the string is an integer, 0 if it is not, -1 if an error is found
*/
int isInteger(char *val);

/* returns 1 if the string is a float, 0 if it is not, -1 if an error is found
*/
int isFloat(char *val);

/* detect the type of column in the csv */
int detectColumnType(int col,CSV *csv);

/* set the types for all columns in the CSV*/
void setTypes(CSV *csv);

/* parses a CSV file given in stdin and saves it in a CSV structure  */
CSV *inputCSV(FILE *inputFile);

/* send CSV structure to stdout */
void outputCSV(CSV *csv, FILE *outputFILE);

/* sorts the csv using the selected column*/
void sortCSV(CSV *csv,char *column);

/* compare two strings using the type to convert them to an integer 
or a float*/
int compare(Data valuea,Data valueb,int type);

/* merge subarrays */
void merge(CSV *a, int start, int n, int m,int col);

/* recursive merge sort */
void mergeSortR(CSV *csv,int start,int n,int col);

/* sorts the csv data using column col using mergesort*/
void mergeSort(CSV *csv,int col);


#endif  /*SORTER_H*/

