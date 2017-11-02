#ifndef DREAM_VARIABLES_H
#define DREAM_VARIABLES_H

#define MEMORYCHECK(x, m)                                                      \
  if (x == NULL) {                                                             \
    printf("%s", m);                                                           \
    exit(1);                                                                   \
  }

struct DREAM_Parameters {
  int n;
  int seq;
  unsigned long ndraw;
  int nCR;
  float Gamma;
  int DEpairs;
  int steps;
  float eps;
  char outlierTest[10];
  float Cb;
  float Wb;
};
struct DREAM_Variables {
  int Nelem;
  float **hist_logp;
  float **pCR;
  float **CR;
  float *lCR;
  float ***Sequences;
  float **Table_JumpRate;
  float *Reduced_Seq;
  unsigned int Iter;
  int counter;
  int teller;
  int new_teller;
  int iloc;
};
struct DREAM_Output {
  float **AR;
  float **outlier;
  float **R_stat;
  float **CR;
};
struct DREAM_Input {
  char pCR[10];
  char reduced_sample_collection[4];
  char InitPopulation[10];
  char BoundHandling[10];
  char save_in_memory[4];
  float Sigma;
  int N;
  int option;
  char DR[4];
  float DRscale;
};
struct Model_Input {
  char ModelName[10];
  int Begin;
  int MaxT;
  int nPar;
  float *ptPET;
  float *ptPrecip;
  float *ptObserved;
  float *ParRangeMin;
  float *ParRangeMax;
  float *IniStates;
};

#endif
