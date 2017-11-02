#ifndef MISC_FUNCTIONS_H
#define MISC_FUNCTIONS_H

enum MVOPS { MVOP_MEAN, MVOP_VAR, MVOP_STD, MVOP_MAX, MVOP_MIN };

void allocate2D(float ***array, int nrows, int ncols);
void deallocate2D(float ***array, int nrows);
void nrandn(float **pArray, int nrows, int ncols);
float sumarray(float *array, int nrows, int ncols);
void randperm(int **array, int n);
void reshape(float **oldarray, int m0, int n0, float ***newarray, int m, int n);
float meanvar(float *arr, int no, MVOPS option);
void transp(float **oldarray, int m0, int n0, float ***newarray,
            bool preallocated = false);
float percentile(float *array, int nelem, float pctile);
void sortarray(float *array, int n, const char *option);
void sortrows(float **array1, int nr, int nc, int sort_col, float **array2);
#endif
