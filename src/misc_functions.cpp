#include "misc_functions.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#ifdef WIN32
#define UNIFORM_RAND (((float)rand()) / RAND_MAX)
#else
#define UNIFORM_RAND (drand48())
#endif

static void siftDown(float **numbers, int sort_col, int nc, int root,
                     int bottom);

void allocate2D(float ***array, int nrows, int ncols) {
  // Function to dynamically allocate 2-dimensional array using malloc.
  // Source: http://www.dreamincode.net/code/snippet1328.htm
  /*  allocate array of pointers  */
  int i;
  *array = (float **)malloc(nrows * sizeof(float *));
  float **vals = *array;
  if (*array == NULL) {
    printf("ERROR in misc_functions.c/allocate2D at line 13: No Memory\n");
    exit(1);
  }
  /*  allocate each row  */
  for (i = 0; i < nrows; i++) {
    (*array)[i] = (float *)malloc(ncols * sizeof(float));
    if ((*array)[i] == NULL) {
      printf("ERROR in misc_functions.c/allocate2D at line 23: No Memory for "
             "array %d\n",
             i);
      exit(1);
    }
  }
  // INFO_LOGF("Allocated %p to %i", *array, nrows);
  for (i = 0; i < nrows; i++) {
    for (int j = 0; j < ncols; j++) {
      vals[i][j] = 0.0;
    }
  }
}

void deallocate2D(float ***array, int nrows) {
  // Corresponding function to dynamically deallocate 2-dimensional array using
  // malloc. Source: http://www.dreamincode.net/code/snippet1328.htm
  /*  deallocate each row  */
  // INFO_LOGF("Deallocating %p to %i", *array, nrows);
  int i;
  for (i = 0; i < nrows; i++) {
    free((*array)[i]);
  }
  /*  deallocate array of pointers  */
  free(*array);
  *array = NULL;
}

void randperm(int **array, int n) {
  int i, j, rnum, key = 1;
  float nf = (float)(n);
  for (i = 0; i < n; i++) {
    rnum = rintf(UNIFORM_RAND * nf);
    if (i == 0) {
      while (rnum == 0) {
        rnum = ceilf(UNIFORM_RAND * nf);
      }
    } else {
      key = 1;
    }
    while (key > 0) {
      key = 0;
      for (j = 0; j < i; j++) {
        if (rnum == (*array)[j] || rnum == 0) {
          key = key + 1;
          rnum = ceilf(UNIFORM_RAND * nf);
        }
      }
    }
    (*array)[i] = rnum;
  }
}

void nrandn(float **pArray, int nrows, int ncols) {
  // Based on algorithm by Dr. Everett (Skip) Carter, Jr.
  // from http://www.taygeta.com/random/gaussian.html

  int i, j;
  float x1, x2, w, y1;
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      do {
        x1 = 2.0 * UNIFORM_RAND - 1.0;
        x2 = 2.0 * UNIFORM_RAND - 1.0;
        w = x1 * x1 + x2 * x2;
      } while (w >= 1.0);
      w = sqrt((-2.0 * log(w)) / w);
      y1 = x1 * w;
      pArray[i][j] = y1;
    }
  }
}

float sumarray(float *array, int nrows, int ncols) {
  int i, idx, cont;
  float suma;
  suma = 0;
  cont = 0;
  if (nrows < 0 || ncols < 0) {
    printf("ERROR: sumarray: number of columns and/or number of rows must be "
           "positive integer. nrows = %d, ncols = %d\n",
           nrows, ncols);
    exit(1);
  }
  if (nrows == 1 || ncols == 1) {
    // 1D array
    if (nrows > ncols) {
      // Column Vector
      idx = nrows;
    } else if (ncols > nrows) {
      // Row Vector
      idx = ncols;
    } else {
      // Single value, i.e. nrows = ncols = 1
      idx = nrows;
    }
    for (i = 0; i < idx; i++) {
      suma = suma + array[i];
    }
  } else {
    // 2D array
    for (i = 0; i < nrows * ncols; i++) {
      // for (j = 0;j < ncols;j++)
      //{
      suma = suma + (array[i] + cont);
      // cont = cont + 1;
      //}
      // cont = cont + 1;
    }
  }
  return (suma);
}

void reshape(float **oldarray, int m0, int n0, float ***newarray, int m,
             int n) {
  int i, j;
  int row, col;
  row = 0;
  col = 0;
  if ((m0 * n0) == (m * n)) {
    allocate2D(newarray, m, n);

    for (j = 0; j < n; j++) {
      for (i = 0; i < m; i++) {
        (*newarray)[i][j] = oldarray[row][col];
        row++;
        if (row == m0) {
          row = 0;
          col++;
        }
      }
    }
  } else {
    printf("ERROR at reshape: m0*n0 must be equal to m*n\n");
    exit(1);
  }
}

float meanvar(float *arr, int no, MVOPS option) {
  int i;
  float var, max = arr[0], min = arr[0];
  float sum = 0.0, sum2 = 0.0, tavg;
  float numArray = (float)no;
  switch (option) {
  case MVOP_MEAN:
    for (i = 0; i < no; i++) {
      sum += arr[i];
    }
    return sum / numArray;
  case MVOP_VAR:
  case MVOP_STD:
    for (i = 0; i < no; i++) {
      sum += arr[i];
    }
    tavg = sum / numArray;

    for (i = 0; i < no; i++) {
      sum2 += pow(arr[i] - tavg, 2.0);
    }
    var = sum2 / (numArray - 1);
    return ((option == MVOP_VAR) ? var : sqrt(var));
  case MVOP_MAX:
    for (i = 0; i < no; i++) {
      if (arr[i] > max) {
        max = arr[i];
      }
    }
    return max;
  case MVOP_MIN:
    for (i = 0; i < no; i++) {
      if (arr[i] < min) {
        min = arr[i];
      }
    }
    return min;
  default:
    return 0; // should never get here
  }
}

void transp(float **oldarray, int m0, int n0, float ***newarray,
            bool preallocated) {
  int i, j;
  if (!preallocated) {
    allocate2D(newarray, n0, m0);
  }
  if (newarray == NULL) {
    printf("ERROR at misc_functions.c at transp: Allocation not successfull\n");
    exit(1);
  }
  for (i = 0; i < m0; i++) {
    for (j = 0; j < n0; j++) {
      (*newarray)[j][i] = oldarray[i][j];
    }
  }
}

void sortarray(float *array, int n, const char *option) {
  float temp;
  int key = 1;
  int i, j;

  // ASCENDING SORTING
  if (strcmp(option, "asc") == 0) {
    while (key > 0) {
      key = 0;
      for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
          if (array[i] > array[j]) {
            temp = array[j];
            array[j] = array[i];
            array[i] = temp;
            key = key + 1;
            break;
          }
        }
      }
    }
  }

  // DESCENDING SORTING
  if (strcmp(option, "des") == 0) {
    while (key > 0) {
      key = 0;
      for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
          if (array[i] < array[j]) {
            temp = array[j];
            array[j] = array[i];
            array[i] = temp;
            key = key + 1;
            break;
          }
        }
      }
    }
  }
}

float percentile(float *array, int nelem, float pctile) {
  // Method to calculate percentiles, alternative by NIST. Source: Wikipedia.
  float n, d, vp, v[nelem];
  int i, k;

  n = ((float)(nelem) / 100) * pctile + 0.5;
  k = (int)(n);
  d = n - k;
  for (i = 0; i < nelem; i++) {
    v[i] = array[i];
  }
  sortarray(&v[0], nelem, "asc");
  if (k == 0) {
    vp = v[0];
  } else if (k == nelem) {
    vp = v[nelem - 1];
  } else {
    vp = v[k] + d * (v[k + 1] - v[k]);
  }
  return (vp);
}

void sortrows(float **array1, int nr, int nc, int sort_col, float **array2) {
  int i, j, ii;
  float temp;

  for (i = 0; i < nr; i++) {
    for (j = 0; j < nc; j++) {
      array2[i][j] = array1[i][j];
    }
  }

  for (i = (nr / 2); i >= 0; i--) {
    siftDown(array2, sort_col, nc, i, nr - 1);
  }

  for (i = nr - 1; i >= 1; i--) {
    for (ii = 0; ii < nc; ii++) {
      temp = array2[0][ii];
      array2[0][ii] = array2[i][ii];
      array2[i][ii] = temp;
    }
    siftDown(array2, sort_col, nc, 0, i - 1);
  }
}

void siftDown(float **numbers, int sort_col, int nc, int root, int bottom) {
  int done, maxChild, ii;
  float temp;

  done = 0;
  while ((root * 2 <= bottom) && (!done)) {
    if (root * 2 == bottom)
      maxChild = root * 2;
    else if (numbers[root * 2][sort_col] > numbers[root * 2 + 1][sort_col])
      maxChild = root * 2;
    else
      maxChild = root * 2 + 1;

    if (numbers[root][sort_col] < numbers[maxChild][sort_col]) {
      for (ii = 0; ii < nc; ii++) {
        temp = numbers[root][ii];
        numbers[root][ii] = numbers[maxChild][ii];
        numbers[maxChild][ii] = temp;
      }
      root = maxChild;
    } else
      done = 1;
  }
}
