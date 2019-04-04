#include "Sorter.h"

/* merge subarrays */
void merge(CSV *a, int start, int n, int m,int col)
{
    int i,j,k;
    CSV *r;

    r= newCSV(n);   /* new csv to save ordered result array */
    for (i=0, j =m, k=0; k<n; k++)
    {
        if(j==n)
            r->rows[k]=a->rows[start+i++];
        else if(i==m) 
            r->rows[k]=a->rows[start+j++];
        else if(compare(a->rows[start+j][col],a->rows[start+i][col],a->types[col])<0)
            r->rows[k]=a->rows[start+j++];
        else
            r->rows[k]=a->rows[start+i++];
    }
    for(i=0; i<n; i++)  /* copy ordered array to old array */
        a->rows[start+i]=r->rows[i];
    free(r->rows);  /* free unused allocated space */
    free(r);
}

/* recursive merge sort */
void mergeSortR(CSV *csv,int start,int n,int col)
{
    int m=n/2;
    if(n<2)
        return;
    mergeSortR(csv,start,m,col);    /* sort lower half*/
    mergeSortR(csv,start+m,n-m,col); /* sort upper half*/
    merge(csv,start,n,m,col);        /* join upper and lower halves in a single ordered array*/
}

/* sorts the csv data using column col using mergesort*/
void mergeSort(CSV *csv,int col)
{
    mergeSortR(csv,0,csv->nrows,col);
}

