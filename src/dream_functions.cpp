// Simple Functions for DREAM
#include "dream_functions.h"
#include "Messages.h"
#include "dream_variables.h"
#include "misc_functions.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef WIN32
#define UNIFORM_RAND ((float)(rand()) / (float)(RAND_MAX))
#else
#define UNIFORM_RAND (drand48())
#endif

void InitVar(struct DREAM_Parameters *pstPar, struct DREAM_Variables **pstRUN,
             struct DREAM_Output **pstOutput) {
  // Initializes important variables for use in the algorithm
  int i, j, zy, zz, buffer;
  float gamma, pstCb, pstWb;
  float ***CRpt;
  // Calculate the parameters in the exponential power density function of Box
  // and Tiao (1973)
  gamma = pstPar->Gamma;
  pstCb = pstPar->Cb;
  pstWb = pstPar->Wb;
  CalcCbWb(&gamma, &pstCb, &pstWb);
  pstPar->Cb = pstCb;
  pstPar->Wb = pstWb;

  buffer =
      10; // Matlab version allows arrays to increase size after pre-allocation

  // Allocate Memory for DREAM run variables
  *pstRUN = (struct DREAM_Variables *)malloc(sizeof(struct DREAM_Variables));
  // Derive the number of elements in the output file
  (*pstRUN)->Nelem = floorf(pstPar->ndraw / pstPar->seq) + 1;
  INFO_LOGF("Nelem is %i (%f), ndraw is %li", (*pstRUN)->Nelem,
            floorf(1.25 * (float)((*pstRUN)->Nelem)), pstPar->ndraw);
  // Allocate the array that contains the history of the log_density of each
  // chain
  allocate2D(&(*pstRUN)->hist_logp, (*pstRUN)->Nelem - 1 + buffer * 2,
             pstPar->seq + 1);
  // Allocate Memory for DREAM output
  *pstOutput = (struct DREAM_Output *)malloc(sizeof(struct DREAM_Output));
  // Initialize output information -- AR
  allocate2D(&(*pstOutput)->AR, (*pstRUN)->Nelem + buffer, 2);
  (*pstOutput)->AR[1][1] = pstPar->seq - 1;
  (*pstOutput)->AR[1][2] = pstPar->seq - 1;
  // Initialize output information -- R statistic
  allocate2D(&(*pstOutput)->R_stat,
             floorf((*pstRUN)->Nelem / pstPar->steps) + buffer, pstPar->n + 1);

  // Outlier
  allocate2D(&(*pstOutput)->outlier, (int)(pstPar->ndraw + 1), 2);

  // if pCR = 'Update'
  // Calculate multinomial probabilities of each of the nCR CR values
  allocate2D(&(*pstRUN)->pCR, 1, pstPar->nCR);
  for (j = 0; j < pstPar->nCR; j++) {
    (*pstRUN)->pCR[0][j] = 1.0f / (float)(pstPar->nCR);
  }
  // Calculate the actual CR values based on p
  allocate2D(&(*pstRUN)->CR, pstPar->seq, pstPar->steps);
  (*pstRUN)->lCR = (float *)malloc(pstPar->nCR * sizeof(float));
  CRpt = &(*pstRUN)->CR;
  GenCR(&pstPar, &(*pstRUN)->pCR, &CRpt);
  // Initialize output information -- N_CR
  allocate2D(&(*pstOutput)->CR,
             floorf((*pstRUN)->Nelem / pstPar->steps) + buffer,
             pstPar->nCR + 1);
  // If save_in_memory == 'yes', malloc 3D array Sequences
  (*pstRUN)->Sequences = (float ***)malloc(
      floorf(1.25 * (float)((*pstRUN)->Nelem)) * sizeof(float **));
  for (i = 0; i < (floorf(1.25 * (float)((*pstRUN)->Nelem))); i++) {
    (*pstRUN)->Sequences[i] =
        (float **)malloc((pstPar->n + 2) * sizeof(float *));
    for (j = 0; j < (pstPar->n + 2); j++) {
      (*pstRUN)->Sequences[i][j] = (float *)malloc(pstPar->seq * sizeof(float));
      memset((*pstRUN)->Sequences[i][j], 0, pstPar->seq * sizeof(float));
    }
  }
  MEMORYCHECK((*pstRUN)->Sequences,
              "ERROR at InitVar: Out of Memory!! 3D array not allocated.\n");
  // Generate the Table with JumpRates (dependent on number of dimensions and
  // number of pairs)
  allocate2D(&(*pstRUN)->Table_JumpRate, pstPar->n, pstPar->DEpairs);
  for (zz = 0; zz < pstPar->DEpairs; zz++) {
    for (zy = 0; zy < pstPar->n; zy++) {
      (*pstRUN)->Table_JumpRate[zy][zz] =
          2.38f / sqrt(2.0f * (float)(zz + 1) * (float)(zy + 1));
    }
  }
  // Initialize Iter and counter
  (*pstRUN)->Iter = pstPar->seq;
  (*pstRUN)->counter = 2;
  (*pstRUN)->iloc = 1;
  (*pstRUN)->teller = 2;
  (*pstRUN)->new_teller = 1;
  // Change MCMCPar.steps to make sure to get nice iteration numbers in first
  // loop
  pstPar->steps = pstPar->steps - 1;
}

void CalcCbWb(float *beta, float *pCb, float *pWb) {
  /*This function calculates the parameters for the exponential power density
  Equation [20] paper by Thiemann et al. WRR 2001, Vol 37, No 10, 2521-2535*/
  float A1, A2;
  A1 = expf(lgammaf(3 * (1 + *beta) / 2));
  A2 = expf(lgammaf((1 + *beta) / 2));
  *pCb = powf((A1 / A2), (1 / (1 + *beta)));
  *pWb = sqrtf(A1) / ((1 + *beta) * powf(A2, (1.5)));
}

void GenCR(struct DREAM_Parameters **ppPar, float ***pppCR, float ****ptCR) {
  // Generates CR values based on current probabilities
  int cont, idx, i_start, i_end, i, j, zz, sumL, *ptL, *ptr;
  int L[(*ppPar)->nCR], L2[(*ppPar)->nCR + 1],
      r[(*ppPar)->seq * (*ppPar)->steps];
  float CRarray[(*ppPar)->seq * (*ppPar)->steps];
  ptL = &L[0];
  ptr = &r[0];
  // How many candidate points for each crossover value?
  multrnd(&ptL, (*ppPar)->seq * (*ppPar)->steps, pppCR, (*ppPar)->nCR, 1);
  sumL = 0;
  for (i = 0; i < ((*ppPar)->nCR + 1); i++) {
    if (i != 0) {
      sumL = sumL + L[i - 1];
      // printf("i %i %i\n", i, L[i-1]);
    }
    L2[i] = sumL;
  }
  // Then select which candidate points are selected with what CR
  randperm(&ptr, (*ppPar)->seq * (*ppPar)->steps);
  // Then generate CR values for each chain
  for (zz = 0; zz < (*ppPar)->nCR; zz++) {
    i_start = L2[zz];
    i_end = L2[zz + 1];
    // printf("start %i, end %i\n", i_start, i_end);
    for (i = i_start; i < i_end; i++) {
      idx = r[i] - 1;
      CRarray[idx] = (float)(zz + 1) / (float)((*ppPar)->nCR);
      // printf("blah %i %i %f\n", zz+1, (*ppPar)->nCR, CRarray[idx]);
    }
  }
  // Reshape CR 1D array into 2D array of "(*ppPar)->seq" rows and
  // "(*ppPar)->steps" columns
  cont = 0;
  for (j = 0; j < (*ppPar)->steps; j++) {
    for (i = 0; i < (*ppPar)->seq; i++) {
      (**ptCR)[i][j] = CRarray[cont];
      cont = cont + 1;
    }
  }
}

void multrnd(int **X, int n, float ***p, int ncols, int m) {
  // MULTRND Multinomial random sequence of m simulations of k outcomes with p
  // probabiltites
  // in n trials.
  int i, j, in;
  float sums;
  float o[n], r[n], s[ncols];
  sums = 0;
  // P = sumarray(**p,1,ncols);
  for (i = 0; i < m; i++) {
    for (in = 0; in < n; in++) {
      o[in] = 1;            // assign 1 to every element of array o
      r[in] = UNIFORM_RAND; // generate Uniformly distributed pseudorandom
                            // numbers [0 1)
    }
    // cumulative sum
    for (in = 0; in < ncols; in++) {
      sums = sums + (*p)[0][in];
      s[in] = sums;
      // printf("index %i s %f\n", in, sums);
      // Initialize Ouput Vector
      (*X)[in] = 0;
    }
    for (j = 0; j < ncols; j++) {
      for (in = 0; in < n; in++) {
        if (s[j] < r[in]) {
          o[in] = o[in] + 1;
        }
      }
    }
    for (j = 0; j < ncols; j++) {
      for (in = 0; in < n; in++) {
        if (o[in] == j + 1) {
          // X - multinomial random deviates (default output).
          (*X)[j] = (*X)[j] + 1;
          // printf("X is %i o is %f %i\n", (*X)[j], o[in], j);
        }
      }
      // Y - multinomial probabilities of the generated random deviates
      // (optional output)  Y[j] = (float)((*X)[j]) / (float)(n);
    }
  }
}

void LHSU(float ***s, int nvar, float *xmax, float *xmin, int nsample) {
  int i, j, *pidx, idx[nsample];
  float P[nsample], ran[nsample][nvar];
  float nsamplef = (float)(nsample);
  // Initialize array ran with random numbers
  for (i = 0; i < nsample; i++) {
    for (j = 0; j < nvar; j++) {
      ran[i][j] = UNIFORM_RAND;
    }
  }
  // Now fill s
  pidx = &idx[0];
  for (j = 0; j < nvar; j++) {
    randperm(&pidx, nsample);
    for (i = 0; i < nsample; i++) {
      P[i] = ((float)(idx[i]) - ran[i][j]) / nsamplef;
      (*s)[i][j] = xmin[j] + P[i] * (xmax[j] - xmin[j]);
    }
  }
}

void InitSequences(float **X, float ***Sequences,
                   struct DREAM_Parameters *MCMCPar) {
  int qq, kk;
  // Initialize sequences
  for (kk = 0; kk < MCMCPar->n + 2; kk++) {
    for (qq = 0; qq < MCMCPar->seq; qq++) {
      Sequences[0][kk][qq] = X[qq][kk];
    }
  }
}

void Gelman(float **R_Stat, int R_Stat_Index, float ***Sequences,
            int numSamples, int numParams, int numChains, int start_loc) {
  // in = number of samples
  // m = number of parameters
  // ip = number of sequences

  // Calculates the R-statistic convergence diagnostic
  int iSamples, iParams, iChains;
  float **meanSeq;
  float **varSeq, *vararray, *B, *W, *sigma2;
  float numChainsf = (float)numChains;
  float numSamplesf = (float)numSamples;
  if (numSamples < 10) {
    // Set the R-statistic to a large value
    for (iParams = 0; iParams < numParams; iParams++) {
      R_Stat[R_Stat_Index][iParams + 1] = -2;
    }
  } else {
    // Step 1: Determine the sequence means
    allocate2D(&meanSeq, numChains, numParams);
    MEMORYCHECK(meanSeq, "at Gelman: Sequences Mean array not allocated\n");
    vararray = (float *)malloc(numSamples * sizeof(float));
    MEMORYCHECK(vararray, "at Gelman: Variance array not allocated\n");
    allocate2D(&varSeq, numChains, numParams);
    MEMORYCHECK(varSeq, "at Gelman: varSeq array not allocated\n");
    for (iChains = 0; iChains < numChains; iChains++) {   // For each sequence
      for (iParams = 0; iParams < numParams; iParams++) { // For each parameter
        for (iSamples = 0; iSamples < numSamples - 1;
             iSamples++) { // For each sample
          vararray[iSamples] =
              Sequences[iSamples + start_loc][iParams][iChains];
        }
        meanSeq[iChains][iParams] =
            meanvar(vararray, numSamples - 1, MVOP_MEAN);
        varSeq[iChains][iParams] = meanvar(vararray, numSamples - 1, MVOP_VAR);
      }
    }
    free(vararray);

    // Step 1: Determine the variance between the sequence means
    vararray = (float *)malloc(numChains * sizeof(float));
    MEMORYCHECK(vararray, "at Gelman: Variance array not allocated\n");
    B = (float *)malloc(numParams * sizeof(float));
    MEMORYCHECK(B, "at Gelman: B array not allocated\n");
    for (iParams = 0; iParams < numParams; iParams++) {   // for each parameter
      for (iChains = 0; iChains < numChains; iChains++) { // for each sequence
        vararray[iChains] = meanSeq[iChains][iParams];
      }
      B[iParams] = numSamplesf * meanvar(vararray, numChains, MVOP_VAR);
    }
    free(vararray);

    // Step 2: Compute the variance of the various sequences

    // Step 2: Calculate the average of the within sequence variances
    W = (float *)malloc(numParams * sizeof(float));
    MEMORYCHECK(W, "at Gelman: W array not allocated\n");
    vararray = (float *)malloc(numChains * sizeof(float));
    MEMORYCHECK(vararray, "at Gelman: Variance array not allocated\n");
    for (iParams = 0; iParams < numParams; iParams++) {
      for (iChains = 0; iChains < numChains; iChains++) {
        vararray[iChains] = varSeq[iChains][iParams];
      }
      W[iParams] = meanvar(vararray, numChains, MVOP_MEAN);
    }
    free(vararray);

    // Step 3: Estimate the target variance
    sigma2 = (float *)malloc(numParams * sizeof(float));
    MEMORYCHECK(sigma2, "at Gelman: sigma2 array not allocated\n");
    for (iParams = 0; iParams < numParams; iParams++) {
      sigma2[iParams] = ((numSamplesf - 1) / numSamplesf) * W[iParams] +
                        (1 / numSamplesf) * B[iParams];
    }

    // Step 5: Compute the R-statistic
    for (iParams = 0; iParams < numParams; iParams++) {
      if (sigma2[iParams] == 0) {
        R_Stat[R_Stat_Index][iParams + 1] = 1.0;
      } else {
        R_Stat[R_Stat_Index][iParams + 1] =
            sqrt((numChainsf + 1) / numChainsf * sigma2[iParams] / W[iParams] -
                 (numSamplesf - 1) / numChainsf / numSamplesf);
      }
    }
    // Deallocating Memory
    free(sigma2);
    free(B);
    free(W);
    deallocate2D(&meanSeq, numChains);
    deallocate2D(&varSeq, numChains);
  }
}

void GetLocation(float **x_old, float *p_old, float *log_p_old, float **X,
                 struct DREAM_Parameters *MCMCPar) {
  // Extracts the current location and density of the chain

  int i, j;

  // First get the current location
  for (i = 0; i < MCMCPar->seq; i++) {
    for (j = 0; j < MCMCPar->n; j++) {
      x_old[i][j] = X[i][j];
    }
  }

  // Then get the current density
  for (i = 0; i < MCMCPar->seq; i++) {
    p_old[i] = X[i][MCMCPar->n];
  }

  // Then get the current logdensity
  for (i = 0; i < MCMCPar->seq; i++) {
    log_p_old[i] = X[i][MCMCPar->n + 1];
  }
}

void DEStrategy(int *DEversion, struct DREAM_Parameters *MCMCPar) {
  // Determine which sequences to evolve with what DE strategy
  int qq, i, j, zend = 0;
  float *p_pair, *Z;
  // Determine probability of selecting a given number of pairs
  p_pair = (float *)malloc((MCMCPar->DEpairs + 1) * sizeof(float));
  MEMORYCHECK(p_pair,
              "ERROR at DEStrategy: Out of Memory!! p_pair not allocated.\n");
  p_pair[0] = 0;
  for (i = 1; i < MCMCPar->DEpairs + 1; i++) {
    p_pair[i] = p_pair[i - 1] + 1.0 / (float)(MCMCPar->DEpairs);
  }
  // Generate a random number between 0 and 1
  Z = (float *)malloc(MCMCPar->seq * sizeof(float));
  MEMORYCHECK(Z, "ERROR at DEStrategy: Out of Memory!! Z not allocated.\n");
  for (i = 0; i < MCMCPar->seq; i++) {
    Z[i] = UNIFORM_RAND;
  }
  // Select number of pairs
  for (qq = 0; qq < MCMCPar->seq; qq++) {
    for (j = 0; j < MCMCPar->DEpairs + 1; j++) {
      if (Z[qq] > p_pair[j]) {
        zend = j + 1;
      }
    }
    DEversion[qq] = zend;
  }
  free(p_pair);
  free(Z);
}

void offde(float **x_new, float **x_old, float **X, float **CR,
           struct DREAM_Parameters *MCMC, float **Table_JumpRate,
           struct Model_Input *Input, const char *BHandling, float *R2,
           const char *DR) {
  // Generates offspring using METROPOLIS HASTINGS monte-carlo markov chain
  int i, j, qq;
  float **eps, D[MCMC->seq][MCMC->n], noise_x[MCMC->seq][MCMC->n],
      delta_x[MCMC->seq][MCMC->n];
  int *DEversion = NULL, *permarray = NULL, tt[MCMC->seq - 1][MCMC->seq],
      idx[MCMC->seq - 1], ii[MCMC->seq], *rr;
  int NrDim, p_i[MCMC->n], *pp_i;
  ;
  float rndnum, Jump_Rate, delta[MCMC->n], temp_delta[MCMC->n];

  memset(delta, 0, MCMC->n * sizeof(float));

  // Generate ergodicity term
  allocate2D(&eps, MCMC->seq, MCMC->n);
  nrandn(eps, MCMC->seq, MCMC->n);
  for (i = 0; i < MCMC->seq; i++) {
    for (j = 0; j < MCMC->n; j++) {
      eps[i][j] =
          1e-6 *
          eps[i][j]; // * (Input->ParRangeMax[j] - Input->ParRangeMin[j]);
      // Generate uniform random numbers for each chain to determine which
      // dimension to update
      D[i][j] = UNIFORM_RAND;

      // Ergodicity for each individual chain
      noise_x[i][j] = MCMC->eps * (2 * UNIFORM_RAND - 1);

      // Initialize the delta update to zero
      delta_x[i][j] = 0;
    }
    ii[i] = 1; // Define ii
  }

  // If not a delayed rejection step --> generate proposal with DE
  if (strcmp(DR, "No") == 0) {
    // Determine which sequences to evolve with what DE strategy
    DEversion = (int *)malloc(MCMC->seq * sizeof(int));
    MEMORYCHECK(DEversion,
                "ERROR at offde: Out of Memory!! DEversion not allocated.\n");
    DEStrategy(DEversion, MCMC);

    // Generate series of permutations of chains
    permarray = (int *)malloc((MCMC->seq - 1) * sizeof(int));
    MEMORYCHECK(permarray,
                "ERROR at offde: Out of Memory!! permarray not allocated.\n");
    for (i = 0; i < MCMC->seq; i++) {
      randperm(&permarray, MCMC->seq - 1);
      for (j = 0; j < MCMC->seq - 1; j++) {
        tt[j][i] = permarray[j] - 1;
      }
    }

    // Each chain evolves using information from other chains to create
    // offspring
    for (qq = 0; qq < MCMC->seq; qq++) {
      // Remove from ii current member as an option
      ii[qq] = 0;
      j = 0;
      for (i = 0; i < MCMC->seq; i++) {
        if (ii[i] > 0) {
          idx[j] = i;
          j = j + 1;
        }
      }

      // randomly select two members of ii that have value == 1
      rr = (int *)malloc(2 * DEversion[qq] * sizeof(int));
      for (i = 0; i < 2 * DEversion[qq]; i++) {
        rr[i] = idx[tt[i][qq]];
      }

      //--- WHICH DIMENSIONS TO UPDATE? DO SOMETHING WITH CROSSOVER ----
      NrDim = 0;
      for (i = 0; i < MCMC->n; i++) {
        if (D[qq][i] > (1 - CR[qq][0])) {
          // printf("Updating dim %i %f %f\n", i, D[qq][i], (1 - CR[qq][0]));
          p_i[NrDim] = i;
          NrDim++; // Determine the number of dimensions that are going to be
                   // updated
        }
      }

      // Update at least one dimension
      if (NrDim == 0) {
        pp_i = &p_i[0];
        randperm(&pp_i, MCMC->n);
        NrDim = 1;
      }

      // Determine the associated JumpRate and compute the jump
      rndnum = UNIFORM_RAND;
      if (rndnum < 0.8) {
        // Lookup Table
        Jump_Rate = Table_JumpRate[NrDim - 1][DEversion[qq] - 1];

        // Produce the difference of the pairs used for population evolution
        for (i = 0; i < DEversion[qq]; i++) {
          for (j = 0; j < MCMC->n; j++) {
            temp_delta[j] = X[rr[i]][j] - X[rr[DEversion[qq] + i]][j];
            delta[j] = delta[j] + temp_delta[j];
          }
        }

        // Then fill update the dimension
        for (i = 0; i < NrDim; i++) {
          delta_x[qq][p_i[i]] =
              (1 + noise_x[qq][p_i[i]]) * Jump_Rate * delta[p_i[i]];
          // printf("delta_x(%i, %i) %f %f %f %f\n", qq, p_i[i],
          // delta_x[qq][p_i[i]], (1 + noise_x[qq][p_i[i]]), Jump_Rate,
          // delta[p_i[i]]);  checker[j] = powf(delta_x[qq][j],2);
        }
      } else {
        // Set the JumpRate to 1 and overwrite CR and DEversion
        Jump_Rate = 1;
        CR[qq][0] = 1;

        // Compute delta from one pair
        for (j = 0; j < MCMC->n; j++) {
          delta[j] = X[rr[0]][j] - X[rr[1]][j];
        }

        // Now jumprate to facilitate jumping from one mode to the other in all
        // dimensions
        for (j = 0; j < MCMC->n; j++) {
          delta_x[qq][j] = Jump_Rate * delta[j];
          // printf("delta_x1(%i, %i) %f\n", qq, j, delta_x[qq][j]);
          // checker[j] = powf(delta_x[qq][j],2);
        }
      }
      ii[qq] = 1;
      free(rr);
    }
  } // if (strcmp(DR,"No")== 0)
  // Update x_old with delta_x and eps
  for (i = 0; i < MCMC->seq; i++) {
    for (j = 0; j < MCMC->n; j++) {
      x_new[i][j] = x_old[i][j] + delta_x[i][j] + eps[i][j];
    }
  }

  // Do boundary handling -- what to do when points fall outside bound
  if (strcmp(BHandling, "Reflect") == 0) {
    ReflectBounds(x_new, Input, MCMC->seq, MCMC->n);
  }

  if (strcmp(BHandling, "Bound") == 0) {
    printf("ERROR at offde: This option has not been implemented2.\n");
    exit(1);
  }

  if (strcmp(BHandling, "Fold") == 0) {
    printf("ERROR at offde: This option has not been implemented3.\n");
    exit(1);
  }

  free(DEversion);
  free(permarray);
  deallocate2D(&eps, MCMC->seq);
}

void ReflectBounds(float **x_new, struct Model_Input *Input, int nmbOfIndivs,
                   int Dim) {
  // Checks the bounds of the parameters
  int i, j;
  // Now check whether points are within bond
  for (i = 0; i < nmbOfIndivs; i++) {
    for (j = 0; j < Dim; j++) {
      if (x_new[i][j] < Input->ParRangeMin[j]) {
        x_new[i][j] = 2 * Input->ParRangeMin[j] - x_new[i][j];
      }

      if (x_new[i][j] > Input->ParRangeMax[j]) {
        x_new[i][j] = 2 * Input->ParRangeMax[j] - x_new[i][j];
      }

      // Now double check if all elements are within bounds
      if (x_new[i][j] < Input->ParRangeMin[j]) {
        x_new[i][j] =
            Input->ParRangeMin[j] +
            UNIFORM_RAND * (Input->ParRangeMax[j] - Input->ParRangeMin[j]);
      }

      if (x_new[i][j] > Input->ParRangeMax[j]) {
        x_new[i][j] =
            Input->ParRangeMin[j] +
            UNIFORM_RAND * (Input->ParRangeMax[j] - Input->ParRangeMin[j]);
      }
    }
  }
}

void metrop(float **newgen, float *alpha, float *accept, float **x, float **p_x,
            float *log_p_x, float **x_old, float *p_old, float *log_p_old,
            struct Model_Input *pointerInput,
            struct DREAM_Parameters *pointerMCMC, int option) {
  // Metropolis rule for acceptance or rejection
  int i, j, NrChains;
  float pre_alpha, Z;
  // Calculate the number of Chains
  NrChains = pointerMCMC->seq;

  // First set newgen to the old positions in X
  for (i = 0; i < pointerMCMC->seq; i++) {
    for (j = 0; j < pointerMCMC->n; j++) {
      newgen[i][j] = x_old[i][j];
    }
    newgen[i][pointerInput->nPar] = p_old[i];
    newgen[i][pointerInput->nPar + 1] = log_p_old[i];
  }

  //-------------------- Now check the various options ----------------------
  if (option == 1) {
    for (i = 0; i < NrChains; i++) {
      pre_alpha = (p_x[i][0] - 1.0) / (p_old[i] - 1.0);
      // printf("p %f %i %f %f\n", pre_alpha, i, p_x[i][0], p_old[i]);
      if (pre_alpha < 1.0) {
        alpha[i] = pre_alpha;
      } else {
        alpha[i] = 1.0;
      }
    }
  }

  if ((option == 2) || (option == 4)) // Lnp probability evaluation
  {
    printf("ERROR at metrop: This option has not been implemented.\n");
    exit(1);
  }

  if (option == 3) // SSE probability evaluation
  {
    for (i = 0; i < NrChains; i++) {
      float expt = (float)(pointerInput->MaxT);
      expt *= -1;
      expt *= ((1 + pointerMCMC->Gamma) / 2);
      pre_alpha = powf(((p_x[i][0] - 1.0) / (p_old[i] - 1.0)), expt);
      // pre_alpha = powf((p_x[i][0]/p_old[i]), expt);
      // printf("Weee %f %f %f %f\n", pre_alpha, expt, p_x[i][0], p_old[i]);
      if (pre_alpha < 1.0) {
        alpha[i] = pre_alpha;
      } else {
        alpha[i] = 1.0;
      }
    }
  }

  if (option == 5) // Similar as 3 but now weighted with Measurement.Sigma
  {
    printf("ERROR at metrop: This option has not been implemented.\n");
    exit(1);
  }

  // Generate random numbers
  for (i = 0; i < NrChains; i++) {
    Z = UNIFORM_RAND;
    // printf("a %f %f %i\n", Z, alpha[i], i);
    if (Z < alpha[i]) // Find which alpha's are greater than Z
    {
      accept[i] = 1; // indicate that these chains have been accepted
      for (j = 0; j < pointerMCMC->n; j++) {
        newgen[i][j] = x[i][j]; // update these chains
      }
      // update these chains
      newgen[i][pointerInput->nPar] = p_x[i][0];
      newgen[i][pointerInput->nPar + 1] = log_p_x[i];
    }
  }
}

void CalcDelta(float *delta_tot, struct DREAM_Parameters *MCMC,
               float *delta_normX, struct DREAM_Variables *RUNvar, int gnum) {
  // Calculate total normalized Euclidean distance for each crossover value
  int zz, i;

  // Derive sum_p2 for each different CR value
  for (zz = 0; zz < MCMC->nCR; zz++) {
    // Find which chains are updated with zz/MCMCPar.nCR
    for (i = 0; i < MCMC->seq; i++) {
      float testVal = (float)(zz + 1) / (float)(MCMC->nCR);
      if (RUNvar->CR[i][gnum] == testVal) {
        // Add the normalized squared distance tot the current delta_tot;
        delta_tot[zz] = delta_tot[zz] + delta_normX[i];
      }
    }
  }
}

void AdaptpCR(struct DREAM_Variables *RUNvar, struct DREAM_Parameters *MCMC,
              float *delta_tot) {
  // Updates the probabilities of the various crossover values

  float **CRvector, sumpCR;
  int i, zz, cont;

  // Make CR to be a single vector
  reshape(RUNvar->CR, MCMC->seq, MCMC->steps, &CRvector, 1,
          MCMC->seq * MCMC->steps);

  // Determine lCR
  for (zz = 0; zz < MCMC->nCR; zz++) {
    // Determine how many times a particular CR value is used
    cont = 0;
    for (i = 0; i < MCMC->seq * MCMC->steps; i++) {
      float testVal = (float)(zz + 1) / (float)(MCMC->nCR);
      if (CRvector[0][i] == testVal) {
        cont = cont + 1;
      }
    }
    // This is used to weight delta_tot
    RUNvar->lCR[zz] = RUNvar->lCR[zz] + cont;
  }

  // Adapt pCR using information from averaged normalized jumping distance
  for (i = 0; i < MCMC->nCR; i++) {
    RUNvar->pCR[0][i] = (float)(MCMC->seq) * (delta_tot[i] / RUNvar->lCR[i]) /
                        sumarray(&delta_tot[0], MCMC->nCR, 1);
  }

  // Normalize pCR
  sumpCR = sumarray(&RUNvar->pCR[0][0], 1, MCMC->nCR);
  for (i = 0; i < MCMC->nCR; i++) {
    RUNvar->pCR[0][i] = RUNvar->pCR[0][i] / sumpCR;
    INFO_LOGF("pCR %i %f %f %f %f", i, RUNvar->pCR[0][i], delta_tot[i],
              RUNvar->lCR[i], sumarray(&delta_tot[0], MCMC->nCR, 1));
  }
  deallocate2D(&CRvector, 1);
}

void RemOutLierChains(float **X, struct DREAM_Variables *RUNvar,
                      struct DREAM_Parameters *MCMC,
                      struct DREAM_Output *output) {
  // Finds outlier chains and removes them when needed
  int i, j, cont, idx_start, idx_end, arr_size, Nid, *chain_id, qq, r_idx = 0;
  float *mean_hist_logp, *col_array, Q1, Q3, IQR, UpperRange;

  // Determine the number of elements of L_density
  idx_end = RUNvar->Nelem - 1;
  idx_start = floorf(0.5 * idx_end);
  arr_size = idx_end - idx_start + 1;

  col_array = (float *)malloc(arr_size * sizeof(float));
  MEMORYCHECK(
      col_array,
      "ERROR at RemOutLierChains: Out of Memory!! col_array not allocated.\n")
  mean_hist_logp = (float *)malloc(MCMC->seq * sizeof(float));
  MEMORYCHECK(mean_hist_logp, "ERROR at RemOutLierChains: Out of Memory!! "
                              "mean_hist_logp not allocated.\n")
  // Then determine the mean log density of the active chains
  for (j = 0; j < MCMC->seq; j++) {
    cont = 0;
    for (i = idx_start; i < idx_end; i++) {
      col_array[cont] = RUNvar->hist_logp[i][j];
      cont++;
    }
    mean_hist_logp[j] = meanvar(col_array, arr_size, MVOP_MEAN);
  }

  // Initialize chain_id and Nid
  Nid = 0;

  // Check whether any of these active chains are outlier chains
  chain_id = (int *)malloc(MCMC->seq * sizeof(int));
  MEMORYCHECK(
      chain_id,
      "ERROR at RemOutLierChains: Out of Memory!! chain_id not allocated.\n");
  if (strcmp(MCMC->outlierTest, "IQR_Test") == 0) {
    // Derive the upper and lower quantile of the data
    Q1 = percentile(mean_hist_logp, MCMC->seq, 75);
    Q3 = percentile(mean_hist_logp, MCMC->seq, 25);

    // Derive the Inter quartile range
    IQR = Q1 - Q3;

    // Compute the upper range -- to detect outliers
    UpperRange = Q3 - 2 * IQR;

    // See whether there are any outlier chains
    cont = 0;
    for (i = 0; i < MCMC->seq; i++) {
      if (mean_hist_logp[i] < UpperRange) {
        chain_id[cont] = i;
        cont = cont + 1;
      }
    }

    Nid = cont;
  }

  if (strcmp(MCMC->outlierTest, "Grubbs_test") == 0) {
    printf("ERROR at RemOutLierChains: Grubbs_test option has not been "
           "implemented\n");
    exit(1);
  }

  if (strcmp(MCMC->outlierTest, "Mahal_test") == 0) {
    printf("ERROR at RemOutLierChains: Mahal_test option has not been "
           "implemented\n");
    exit(1);
  }

  if (Nid > 0) {
    INFO_LOGF("Killing outlier chain!! %i", RUNvar->Iter);
    // Loop over each outlier chain
    for (qq = 0; qq < Nid; qq++) {
      // Draw random other chain -- cannot be the same as current chain
      for (i = 0; i < MCMC->seq; i++) {
        if (mean_hist_logp[i] == meanvar(mean_hist_logp, MCMC->seq, MVOP_MAX)) {
          r_idx = i;
          break;
        }
      }

      // Added -- update hist_logp -- chain will not be considered as an outlier
      // chain then
      for (i = 0; i < RUNvar->Nelem - 1; i++) {
        RUNvar->hist_logp[i][chain_id[qq]] = RUNvar->hist_logp[i][r_idx];
      }

      // Jump outlier chain to r_idx -- Sequences
      // Jump outlier chain to r_idx -- X
      for (i = 0; i < MCMC->n + 2; i++) {
        RUNvar->Sequences[0][i][chain_id[qq]] = X[r_idx][i];
        X[chain_id[qq]][i] = X[r_idx][i];
      }

      // Add to chainoutlier
      output->outlier[RUNvar->Iter][0] = RUNvar->Iter;
      output->outlier[RUNvar->Iter][1] = chain_id[qq];
    }
  }

  free(col_array);
  free(mean_hist_logp);
  free(chain_id);
}

void GenParSet(float **ParSet, struct DREAM_Variables *RUNvar,
               int post_Sequences, struct DREAM_Parameters *MCMC, FILE *fid,
               float *bestParams) {
  int qq, i, j, cont = 0, num;
  float **parset, **sorted;
  // Generates ParSet
  // If save in memory -> No -- ParSet is empty
  // if (post_Sequences == 1)
  //{
  // Do nothing
  //}
  // else
  //{
  // If save in memory -> Yes -- ParSet derived from all sequences
  allocate2D(&parset, post_Sequences * MCMC->seq, MCMC->n + 3);
  allocate2D(&sorted, post_Sequences * MCMC->seq, MCMC->n + 3);
  cont = 0;
  for (qq = 0; qq < MCMC->seq; qq++) {
    num = 0;
    for (i = 0; i < post_Sequences; i++) {
      for (j = 0; j < MCMC->n + 2; j++) {
        parset[cont][j] = RUNvar->Sequences[i][j][qq];
      }
      parset[cont][MCMC->n + 2] = num;
      cont = cont + 1;
      num = num + 1;
    }
  }
  //}
  // Sort according to MATLAB DREAM
  sortrows(parset, cont, MCMC->n + 3, MCMC->n + 2, sorted);

  // Sort so the best parameter set is the last
  sortrows(sorted, cont, MCMC->n + 3, MCMC->n, parset);
  // Write to disk
  for (i = 0; i < (MCMC->seq * post_Sequences); i++) {
    for (j = 0; j < MCMC->n + 2; j++) {
      // ParSet[i][j] = sorted[i][j];
      ParSet[i][j] = parset[i][j];
      if (j == MCMC->n + 1) {
        fprintf(fid, "%f\n", ParSet[i][j]);
      } else {
        fprintf(fid, "%f,", ParSet[i][j]);
      }
    }
  }
  i = (MCMC->seq * post_Sequences) - 1;
  for (j = 0; j < MCMC->n; j++) {
    bestParams[j] = ParSet[i][j];
  }

  deallocate2D(&parset, post_Sequences * MCMC->seq);
  deallocate2D(&sorted, post_Sequences * MCMC->seq);
}
