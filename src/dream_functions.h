#ifndef DREAM_FUNCTIONS_H
#define DREAM_FUNCTIONS_H

#include <cstdio>

void dream(struct DREAM_Parameters *pointerMCMC,
           struct Model_Input *pointerInput);
void CalcCbWb(float *beta, float *pCb, float *pWb);
void InitVar(struct DREAM_Parameters *pstPar, struct DREAM_Variables **ptRUN,
             struct DREAM_Output **pstOutput);
void GenCR(struct DREAM_Parameters **ppPar, float ***pppCR, float ****ptCR);
void multrnd(int **X, int n, float ***p, int ncols, int m);
void LHSU(float ***s, int nvar, float *xmax, float *xmin, int nsample);
void InitSequences(float **X, float ***Sequences,
                   struct DREAM_Parameters *MCMCPar);
void Gelman(float **R_Stat, int R_Stat_Index, float ***Sequences,
            int numSamples, int numParams, int numChains, int start_loc);
void GetLocation(float **x_old, float *p_old, float *log_p_old, float **X,
                 struct DREAM_Parameters *MCMCPar);
void offde(float **x_new, float **x_old, float **X, float **CR,
           struct DREAM_Parameters *MCMC, float **Table_JumpRate,
           struct Model_Input *Input, const char *BHandling, float *R2,
           const char *DR);
void DEStrategy(int *DEversion, struct DREAM_Parameters *MCMCPar);
void ReflectBounds(float **x_new, struct Model_Input *Input, int nmbOfIndivs,
                   int Dim);
void metrop(float **newgen, float *alpha, float *accept, float **x, float **p_x,
            float *log_p_x, float **x_old, float *p_old, float *log_p_old,
            struct Model_Input *pointerInput,
            struct DREAM_Parameters *pointerMCMC, int option);
void CalcDelta(float *delta_tot, struct DREAM_Parameters *MCMC,
               float *delta_normX, struct DREAM_Variables *RUNvar, int gnum);
void AdaptpCR(struct DREAM_Variables *RUNvar, struct DREAM_Parameters *MCMC,
              float *delta_tot);
void RemOutLierChains(float **X, struct DREAM_Variables *RUNvar,
                      struct DREAM_Parameters *MCMC,
                      struct DREAM_Output *output);
void GenParSet(float **ParSet, struct DREAM_Variables *RUNvar,
               int post_Sequences, struct DREAM_Parameters *MCMC, FILE *fid,
               float *bestParams);
#endif
