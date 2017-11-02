#include "Messages.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <sys/time.h>
#if _OPENMP
#include <omp.h>
#endif
#include "DREAM.h"

void DREAM::Initialize(CaliParamConfigSection *caliParamConfigNew,
                       RoutingCaliParamConfigSection *routingCaliParamConfigNew,
                       SnowCaliParamConfigSection *snowCaliParamConfigNew,
                       int numParamsWBNew, int numParamsRNew, int numParamsSNew,
                       Simulator *simNew) {
  caliParamConfig = caliParamConfigNew;
  routingCaliParamConfig = routingCaliParamConfigNew;
  snowCaliParamConfig = snowCaliParamConfigNew;
  numParamsWB = numParamsWBNew;
  numParamsR = numParamsRNew;
  numParamsS = numParamsSNew;
  numParams = numParamsWBNew + numParamsRNew + numParamsSNew;
  sim = simNew;
  isEnsemble = false;

  // Stuff from CaliParamConfigSection
  goal = objectiveGoals[caliParamConfig->GetObjFunc()];
  objectiveString = objectiveStrings[caliParamConfig->GetObjFunc()];

  /*
   // Configurable Parameters
   topNum = caliParamConfig->ARSGetTopNum();
   minObjScore = caliParamConfig->ARSGetCritObjScore();
   convergenceCriteria = caliParamConfig->ARSGetConvCriteria();
   burnInSets = caliParamConfig->ARSGetBurnInSets();

   // Initialize vars & RNG
   totalSets = 0;
   goodSets = 0;*/
#ifdef WIN32
  srand(time(NULL));
#else
  srand48(time(NULL));
#endif

  minParams = new float[numParams];
  maxParams = new float[numParams];

  INFO_LOGF("num params is %i (water balance :%i, routing: %i, snow: %i)",
            numParams, numParamsWB, numParamsR, numParamsS);

  // Stuff from CaliParamConfigSection

  memcpy(minParams, caliParamConfig->GetParamMins(),
         sizeof(float) * numParamsWB);
  memcpy(maxParams, caliParamConfig->GetParamMaxs(),
         sizeof(float) * numParamsWB);
  memcpy(&(minParams[numParamsWB]), routingCaliParamConfig->GetParamMins(),
         sizeof(float) * numParamsR);
  memcpy(&(maxParams[numParamsWB]), routingCaliParamConfig->GetParamMaxs(),
         sizeof(float) * numParamsR);
  if (numParamsS > 0) {
    memcpy(&(minParams[numParamsWB + numParamsR]),
           snowCaliParamConfig->GetParamMins(), sizeof(float) * numParamsS);
    memcpy(&(maxParams[numParamsWB + numParamsR]),
           snowCaliParamConfig->GetParamMaxs(), sizeof(float) * numParamsS);
  }

  // srand48(0);
  pointerMCMC = new DREAM_Parameters();
  pointerInput = new Model_Input();

  // DREAM Parameters
  pointerMCMC->n = numParams;            // Dimension of the problem
  pointerMCMC->seq = pointerMCMC->n + 0; // Number of Markov Chains / sequences
  pointerMCMC->ndraw =
      caliParamConfig
          ->DREAMGetNDraw(); // Maximum number of function evaluations
  pointerMCMC->nCR =
      3; // Crossover values used to generate proposals (geometric series)
  pointerMCMC->Gamma = 0;   // Kurtosis parameter Bayesian Inference Scheme
  pointerMCMC->DEpairs = 3; // Number of DEpairs, only 1 or 2? 3 crashes
  pointerMCMC->steps = 10;  // Number of steps in sem
  pointerMCMC->eps = 2e-1;  // Random error for ergodicity
  strcpy(pointerMCMC->outlierTest,
         "IQR_Test"); // What kind of test to detect outlier chains?

  float newdraw = floorf((float)pointerMCMC->ndraw /
                             (float)(pointerMCMC->seq * pointerMCMC->steps) +
                         0.5) *
                  (pointerMCMC->seq * pointerMCMC->steps);
  pointerMCMC->ndraw = newdraw;

  pointerInput->MaxT = (int)simNew->GetNumSteps();
  pointerInput->nPar =
      numParams; // Number of Parameters = Dimension of the problem
  pointerInput->ParRangeMin = minParams;
  pointerInput->ParRangeMax = maxParams;
}

void DREAM::Initialize(CaliParamConfigSection *caliParamConfigNew,
                       int numParamsNew, float *paramMins, float *paramMaxs,
                       std::vector<Simulator> *ensSimsNew,
                       std::vector<int> *paramsPerSimNew) {
  caliParamConfig = caliParamConfigNew;
  numParams = numParamsNew;
  ensSims = ensSimsNew;
  paramsPerSim = paramsPerSimNew;
  isEnsemble = true;

  // Stuff from CaliParamConfigSection
  goal = objectiveGoals[caliParamConfig->GetObjFunc()];
#ifdef WIN32
  srand(time(NULL));
#else
  srand48(time(NULL));
#endif
  pointerMCMC = new DREAM_Parameters();
  pointerInput = new Model_Input();
  // DREAM Parameters
  pointerMCMC->n = numParams;
  pointerMCMC->seq = pointerMCMC->n + 0;
  pointerMCMC->ndraw = caliParamConfig->DREAMGetNDraw();
  pointerMCMC->nCR = 3;
  pointerMCMC->Gamma = 0;
  pointerMCMC->DEpairs = 3;
  pointerMCMC->steps = 10;
  pointerMCMC->eps = 2e-1;
  strcpy(pointerMCMC->outlierTest, "IQR_Test");

  float newdraw = floorf((float)pointerMCMC->ndraw /
                             (float)(pointerMCMC->seq * pointerMCMC->steps) +
                         0.5) *
                  (pointerMCMC->seq * pointerMCMC->steps);
  pointerMCMC->ndraw = newdraw;

  pointerInput->MaxT = (int)ensSimsNew->at(0).GetNumSteps();
  pointerInput->nPar = numParams;
  pointerInput->ParRangeMin = paramMins;
  pointerInput->ParRangeMax = paramMaxs;
}

void DREAM::CalibrateParams() {
  //------VARIABLES
  //BLOCK----------------------------------------------------------//
  int i, j, gen_number, ItExtra, start_loc, end_loc;
  float *delta_tot, *R2 = NULL, c_std, **r, *std_array, *dnX_array,
                    *delta_normX, *post_array;
  float **x, **X, **x_old, **x_new, **newgen, **t_newgen;
  float **p, *log_p, *p_old, *log_p_old, **p_xnew, *log_p_xnew, *alpha12,
      *accept, ***CRpt;
  post_Sequences = 1;
  struct DREAM_Output *pointerOutput;
  bool converged = false;

  // steps = pointerMCMC->steps; //Steps will change, need to kepp original
  // value for memory deallocation
  //------Start
  //Messages-----------------------------------------------------------//
  INFO_LOGF("%s",
            "DiffeRential Evolution Adaptive Metropolis (DREAM) Algorithm");

#if _OPENMP
  if (omp_get_max_threads() > 1) {
    INFO_LOGF("Running in parallel mode with %i threads",
              omp_get_max_threads());
  } else {
    INFO_LOGF("%s", "Running in sequential Mode");
  }
#else
  INFO_LOGF("%s", "Running in sequential Mode");
#endif

  //------Initialize Variables: Call InitVar
  //routine-------------------------------//
  InitVar(pointerMCMC, &pointerRUNvar, &pointerOutput);
  //------Check for Successful Memory
  //Allocation-----------------------------------//
  MEMORYCHECK(pointerRUNvar, "at dream.c: Memory Allocation for DREAM struct "
                             "run variable not successfull\n");
  MEMORYCHECK(pointerRUNvar->hist_logp, "at dream.c: Memory Allocation for "
                                        "DREAM run variable hist_logp not "
                                        "successfull\n");
  MEMORYCHECK(pointerRUNvar->pCR, "at dream.c: Memory Allocation for DREAM run "
                                  "variable pCR not successfull\n");
  MEMORYCHECK(pointerRUNvar->CR, "at dream.c: Memory Allocation for DREAM run "
                                 "variable CR not successfull\n");
  MEMORYCHECK(pointerRUNvar->lCR, "at dream.c: Memory Allocation for DREAM run "
                                  "variable lCR not successfull\n");
  MEMORYCHECK(pointerOutput, "at dream.c: Memory Allocation for DREAM struct "
                             "output variable not successfull\n");
  MEMORYCHECK(pointerOutput->AR, "at dream.c: Memory Allocation for DREAM "
                                 "output variable AR not successfull\n");
  MEMORYCHECK(pointerOutput->CR, "at dream.c: Memory Allocation for DREAM "
                                 "output variable CR not successfull\n");
  MEMORYCHECK(pointerOutput->outlier, "at dream.c: Memory Allocation for DREAM "
                                      "output variable outlier not "
                                      "successfull\n");
  MEMORYCHECK(pointerRUNvar->Sequences, "at dream.c: Memory Allocation for "
                                        "DREAM run variable Sequences not "
                                        "successfull\n");
  MEMORYCHECK(pointerRUNvar->Table_JumpRate,
              "at dream.c: Memory Allocation for DREAM run variable "
              "Table_JumpRate not successfull\n");
  //------Step 1: Sample s points in the parameter
  //space---------------------------//  if Extra.InitPopulation = 'LHS_BASED'
  // Latin hypercube sampling when indicated
  allocate2D(&x, pointerMCMC->seq, pointerInput->nPar);
  LHSU(&x, pointerInput->nPar, pointerInput->ParRangeMax,
       pointerInput->ParRangeMin, pointerMCMC->seq);

  // Step 2: Calculate posterior density associated with each value in x
  allocate2D(&p, pointerMCMC->seq, 2);
  MEMORYCHECK(
      p,
      "at dream.c: Memory Allocation for DREAM variable p not successfull\n");
  log_p = (float *)malloc(pointerMCMC->seq * sizeof(float));
  MEMORYCHECK(log_p, "at dream.c: Memory Allocation for DREAM variable log_p "
                     "not successfull\n");
  CompDensity(p, log_p, x, pointerMCMC, pointerInput, 3);

  // Save the initial population, density and log density in one matrix X
  allocate2D(&X, pointerMCMC->seq, pointerMCMC->n + 2);
  MEMORYCHECK(
      X,
      "at dream.c: Memory Allocation for DREAM variable X not successfull\n");
  for (i = 0; i < pointerMCMC->seq; i++) {
    for (j = 0; j < pointerMCMC->n; j++) {
      X[i][j] = x[i][j];
    }
    X[i][pointerInput->nPar] = p[i][0];
    X[i][pointerInput->nPar + 1] = log_p[i];
  }

  // Then initialize the sequences
  // if save in memory = Yes
  InitSequences(X, pointerRUNvar->Sequences, pointerMCMC);

  // Reduced sample collection if reduced_sample_collection = Yes
  // iloc_2 = 0;

  // Save N_CR in memory and initialize delta_tot
  pointerOutput->CR[0][0] = pointerRUNvar->Iter;
  for (i = 1; i < pointerMCMC->nCR + 1; i++) {
    pointerOutput->CR[0][i] = pointerRUNvar->pCR[0][i - 1];
  }
  delta_tot = (float *)malloc(pointerMCMC->nCR * sizeof(float));
  MEMORYCHECK(delta_tot, "at dream.c: Memory Allocation for DREAM variable "
                         "delta_tot not successfull\n");
  for (i = 0; i < pointerMCMC->nCR; i++) {
    delta_tot[i] = 0.0;
  }

  // Save history log density of individual chains
  pointerRUNvar->hist_logp[0][0] = pointerRUNvar->Iter;
  for (i = 1; i < pointerMCMC->seq + 1; i++) {
    pointerRUNvar->hist_logp[0][i] = X[i - 1][pointerMCMC->n + 1];
  }
  // Compute the R-statistic
  pointerOutput->R_stat[0][0] = pointerRUNvar->Iter;
  Gelman(pointerOutput->R_stat, 0, pointerRUNvar->Sequences,
         pointerRUNvar->iloc, pointerMCMC->n, pointerMCMC->seq, 0);

  // Allocate Variables
  allocate2D(&x_old, pointerMCMC->seq, pointerMCMC->n);
  MEMORYCHECK(x_old, "at dream.c: Memory Allocation for DREAM variable x_old "
                     "not successfull\n");
  p_old = (float *)malloc(pointerMCMC->seq * sizeof(float *));
  MEMORYCHECK(p_old, "at dream.c: Memory Allocation for DREAM variable p_old "
                     "not successfull\n");
  log_p_old = (float *)malloc(pointerMCMC->seq * sizeof(float *));
  MEMORYCHECK(log_p_old, "at dream.c: Memory Allocation for DREAM variable "
                         "log_p_old not successfull\n");
  allocate2D(&x_new, pointerMCMC->seq, pointerMCMC->n);
  MEMORYCHECK(x_new, "at dream.c: Memory Allocation for DREAM variable x_new "
                     "not successfull\n");
  allocate2D(&p_xnew, pointerMCMC->seq, 2);
  MEMORYCHECK(p_xnew, "at dream.c: Memory Allocation for DREAM variable p_xnew "
                      "not successfull\n");
  log_p_xnew = (float *)malloc(pointerMCMC->seq * sizeof(float));
  MEMORYCHECK(log_p_xnew, "at dream.c: Memory Allocation for DREAM variable "
                          "log_p_xnew not successfull\n");
  alpha12 = (float *)malloc(pointerMCMC->seq * sizeof(float *));
  MEMORYCHECK(alpha12, "at dream.c: Memory Allocation for DREAM variable "
                       "alpha12 not successfull\n");
  accept = (float *)malloc(pointerMCMC->seq * sizeof(float *));
  MEMORYCHECK(accept, "at dream.c: Memory Allocation for DREAM variable accept "
                      "not successfull\n");

  // Preallocate memory needed here
  allocate2D(&newgen, pointerMCMC->seq, pointerMCMC->n + 2);
  MEMORYCHECK(newgen, "at dream.c: Memory Allocation for DREAM variable newgen "
                      "not successfull\n");
  allocate2D(&r, pointerMCMC->seq, pointerMCMC->n + 2);
  MEMORYCHECK(
      r,
      "at dream.c: Memory Allocation for DREAM variable r not successfull\n");
  std_array = (float *)malloc(pointerMCMC->seq * sizeof(float));
  MEMORYCHECK(std_array, "at dream.c: Memory Allocation for DREAM variable "
                         "std_array not successfull\n");
  delta_normX = (float *)malloc(pointerMCMC->seq * sizeof(float));
  MEMORYCHECK(delta_normX, "at dream.c: Memory Allocation for DREAM variable "
                           "delta_normX not successfull\n");
  dnX_array = (float *)malloc((pointerMCMC->n + 2) * sizeof(float));
  MEMORYCHECK(dnX_array, "at dream.c: Memory Allocation for DREAM variable "
                         "dnX_array not successfull\n");
  allocate2D(&t_newgen, pointerMCMC->n + 2, pointerMCMC->seq);

#ifdef _WIN32
  setIteration(0);
#endif

  // Now start iteration ...
  while ((pointerRUNvar->Iter < pointerMCMC->ndraw) && !converged) {
    // Loop a number of times
    for (gen_number = 0; gen_number < pointerMCMC->steps; gen_number++) {
      // Initialize DR properties
      // accept2 = 0;
      ItExtra = 0;
      pointerRUNvar->new_teller = pointerRUNvar->new_teller + 1;

      // Define the current locations and associated posterior densities
      GetLocation(x_old, p_old, log_p_old, X, pointerMCMC);

      // Now generate candidate in each sequence using current point and members
      // of X
      offde(x_new, x_old, X, pointerRUNvar->CR, pointerMCMC,
            pointerRUNvar->Table_JumpRate, pointerInput, "Reflect", R2, "No");

      // Now compute the likelihood of the new points
      CompDensity(p_xnew, log_p_xnew, x_new, pointerMCMC, pointerInput, 3);

      // Now apply the acceptance/rejectance rule for the chain itself
      metrop(newgen, alpha12, accept, x_new, p_xnew, log_p_xnew, x_old, p_old,
             log_p_old, pointerInput, pointerMCMC, 3);

      // Check whether we do delayed rejection or not
      // If DR = "Yes", then do compute several things. For this implementation
      // of DREAM,  We skipped said computations (i.e. DR = "No")

      // Define the location in the sequence
      // if strcmp(Extra.save_in_memory,'Yes');
      pointerRUNvar->iloc = pointerRUNvar->iloc + 1;

      // Now update the locations of the Sequences with the current locations
      transp(newgen, pointerMCMC->seq, pointerMCMC->n + 2, &t_newgen, true);
      for (i = 0; i < pointerMCMC->n + 2; i++) {
        for (j = 0; j < pointerMCMC->seq; j++) {
          pointerRUNvar->Sequences[pointerRUNvar->iloc - 1][i][j] =
              t_newgen[i][j];
        }
      }

      // And update X using current members of Sequences
      for (i = 0; i < pointerMCMC->n + 2; i++) {
        for (j = 0; j < pointerMCMC->seq; j++) {
          X[j][i] = newgen[j][i];
        }
      }

      // if strcmp(Extra.pCR,'Update');
      // Calculate the standard deviation of each dimension of X

      for (j = 0; j < pointerMCMC->n + 2; j++) {
        for (i = 0; i < pointerMCMC->seq; i++) {
          std_array[i] = X[i][j];
        }
        c_std = meanvar(std_array, pointerMCMC->seq, MVOP_STD);
        for (i = 0; i < pointerMCMC->seq; i++) {
          r[i][j] = c_std;
        }
      }

      for (i = 0; i < pointerMCMC->seq; i++) {
        for (j = 0; j < pointerMCMC->n + 2; j++) {
          dnX_array[j] = powf((x_old[i][j] - X[i][j]) / r[i][j], 2);
        }
        delta_normX[i] = sumarray(dnX_array, pointerMCMC->n + 2, 1);
      }

      // Use this information to update sum_p2 to update N_CR
      CalcDelta(delta_tot, pointerMCMC, delta_normX, pointerRUNvar, gen_number);

      // Update hist_logp
      pointerRUNvar->hist_logp[pointerRUNvar->counter - 1][0] =
          pointerRUNvar->Iter;

      for (j = 1; j < pointerMCMC->seq + 1; j++) {
        pointerRUNvar->hist_logp[pointerRUNvar->counter - 1][j] =
            X[j - 1][pointerMCMC->n + 1];
      }

      // Save some important output -- Acceptance Rate
      pointerOutput->AR[pointerRUNvar->counter - 1][0] = pointerRUNvar->Iter;
      // Next Line, variable accept2 not included in this implementation of
      // DREAM since Extra.DR = 'No'
      pointerOutput->AR[pointerRUNvar->counter - 1][1] =
          100.0 * (sumarray(accept, pointerMCMC->seq, 1) /
                   (float)((pointerMCMC->seq + ItExtra)));

      // Update Iteration and counter
      pointerRUNvar->Iter = pointerRUNvar->Iter + pointerMCMC->seq + ItExtra;
#ifdef _WIN32
      setIteration(pointerRUNvar->Iter);
#else
      if ((pointerRUNvar->Iter % 400) == 0) {
        INFO_LOGF("Completed %i simulations so far!", pointerRUNvar->Iter);
      }
#endif
      pointerRUNvar->counter = pointerRUNvar->counter + 1;
    } // for (gen_number=0;gen_number < 1;gen_number++)
    //-----------End of: for (gen_number=0;gen_number <
    //1;gen_number++)----------------------- //

    // Store Important Diagnostic information -- Probability of individual
    // crossover values
    pointerOutput->CR[pointerRUNvar->teller - 1][0] = pointerRUNvar->Iter;
    for (i = 1; i < pointerMCMC->nCR + 1; i++) {
      pointerOutput->CR[pointerRUNvar->teller - 1][i] =
          pointerRUNvar->pCR[0][i - 1];
    }

    // Do this to get rounded iteration numbers
    if (pointerRUNvar->teller == 2) {
      pointerMCMC->steps = pointerMCMC->steps + 1;
    }

    // Check whether to update individual pCR values
    if (pointerRUNvar->Iter <= (0.1 * pointerMCMC->ndraw)) {
      // if strcmp(Extra.pCR,'Update');
      // Update pCR values
      AdaptpCR(pointerRUNvar, pointerMCMC, delta_tot);
    } else {
      // See whether there are any outlier chains, and remove them to current
      // best value of X
      RemOutLierChains(X, pointerRUNvar, pointerMCMC, pointerOutput);
    }

    // if strcmp(Extra.pCR,'Update');
    // Generate CR values based on current pCR values
    CRpt = &(pointerRUNvar)->CR;
    GenCR(&pointerMCMC, &(pointerRUNvar)->pCR, &CRpt);

    // Calculate Gelman and Rubin convergence diagnostic
    // if strcmp(Extra.save_in_memory,'Yes');
    if (floorf(0.5 * pointerRUNvar->iloc) > 1) {
      start_loc = floorf(0.5 * pointerRUNvar->iloc);
    } else {
      start_loc = 1;
    }
    end_loc = pointerRUNvar->iloc;

    // Compute the R-statistic using 50% burn-in from Sequences
    Gelman(pointerOutput->R_stat, pointerRUNvar->teller - 1,
           pointerRUNvar->Sequences, (end_loc - start_loc + 1), pointerMCMC->n,
           pointerMCMC->seq, start_loc);
    pointerOutput->R_stat[pointerRUNvar->teller - 1][0] = pointerRUNvar->Iter;

    // Lets see if we converged or not
    converged = true;
    NORMAL_LOGF("R_stat: %f ",
                pointerOutput->R_stat[pointerRUNvar->teller - 1][0]);
    for (j = 0; j < pointerMCMC->n; j++) {
      NORMAL_LOGF("%f ",
                  pointerOutput->R_stat[pointerRUNvar->teller - 1][j + 1]);
      if (pointerOutput->R_stat[pointerRUNvar->teller - 1][j + 1] > 1.2 ||
          pointerOutput->R_stat[pointerRUNvar->teller - 1][j + 1] == -2.0) {
        converged = false;
      }
    }
    NORMAL_LOGF("%s", "\n");
    if (converged) {
      INFO_LOGF("%s", "DREAM has converged on a solution!");
    }
    // Update the Teller
    pointerRUNvar->teller = pointerRUNvar->teller + 1;
  } // while (pointerRUNvar->Iter < pointerMCMC->ndraw)

  // Deallocate preallocated memory here
  deallocate2D(&t_newgen, pointerMCMC->seq);
  deallocate2D(&newgen, pointerMCMC->seq);
  deallocate2D(&r, pointerMCMC->seq);
  free(std_array);
  free(delta_normX);
  free(dnX_array);

  // Postprocess output from DREAM before returning arguments
  post_array = (float *)malloc((pointerMCMC->n + 2) * sizeof(float));
  MEMORYCHECK(post_array, "at dream.c: Memory Allocation for DREAM variable "
                          "post_array not successfull\n");
  for (i = 0; i < floorf(1.25 * pointerRUNvar->Nelem); i++) {
    for (j = 0; j < pointerMCMC->n + 2; j++) {
      post_array[j] = pointerRUNvar->Sequences[i][j][0];
    }
    if (sumarray(post_array, pointerMCMC->n + 2, 1) == 0) {
      post_Sequences = i - 1;
      break;
    }
  }
  free(post_array);

  /*
   for (i = 0; i < pointerRUNvar->teller - 1; i++) {
   printf("T: %i ", i);
   for (j = 0; j < pointerMCMC->n + 1; j++) {
   printf("%f ", pointerOutput->R_stat[i][j]);
   }
   printf("\n");
   }*/

  /*
   //Generate a 2D matrix with samples
   allocate2D(&ParSet, post_Sequences*pointerMCMC->seq, pointerMCMC->n+2);
   GenParSet(ParSet, pointerRUNvar, post_Sequences, pointerMCMC);

   //------Deallocating
   Memory------------------------------------------------------// for (i = 0; i
   < floorf(1.25 * pointerRUNvar->Nelem); i++) { for (j=0; j < pointerMCMC->n+2;
   j++) { free(pointerRUNvar->Sequences[i][j]);
   }
   free(pointerRUNvar->Sequences[i]);
   }
   free(pointerRUNvar->Sequences);

   deallocate2D(&ParSet, post_Sequences*pointerMCMC->seq);
   */

  /*deallocate2D(&pointerRUNvar->hist_logp, pointerRUNvar->Nelem - 1 + 10);
   deallocate2D(&pointerRUNvar->pCR, 1);
   deallocate2D(&pointerRUNvar->CR, pointerMCMC->seq);
   deallocate2D(&pointerRUNvar->Table_JumpRate, pointerMCMC->n);
   deallocate2D(&pointerOutput->AR, pointerRUNvar->Nelem+10);
   deallocate2D(&pointerOutput->CR, floorf(pointerRUNvar->Nelem / steps) + 10);
   deallocate2D(&pointerOutput->outlier, pointerMCMC->ndraw);
   deallocate2D(&pointerOutput->R_stat, floorf((pointerRUNvar->Nelem+10) /
   steps)); free(pointerRUNvar->lCR); free(pointerOutput);
   //free(pointerRUNvar);
   deallocate2D(&x, pointerMCMC->seq);
   deallocate2D(&X, pointerMCMC->seq);
   deallocate2D(&p, pointerMCMC->seq);
   deallocate2D(&p_xnew, pointerMCMC->seq);
   deallocate2D(&x_old, pointerMCMC->seq);
   deallocate2D(&x_new, pointerMCMC->seq);
   free(log_p);
   free(log_p_xnew);
   free(alpha12);
   free(accept);
   free(delta_tot);
   free(p_old);
   free(log_p_old);*/
  INFO_LOGF("%s", "End of DREAM Routine");
}

void DREAM::WriteOutput(char *outputFile, MODELS model, ROUTES route,
                        SNOWS snow) {
  FILE *file = fopen(outputFile, "w");
  int i;
  float **ParSet;
  float *bestParams = new float[pointerMCMC->n];

  for (i = 0; i < numModelParams[model]; i++) {
    fprintf(file, "%s%s", (i == 0) ? "" : ",", modelParamStrings[model][i]);
  }
  int endi = numModelParams[model] + numRouteParams[route];
  for (i = numModelParams[model]; i < endi; i++) {
    fprintf(file, ",%s", routeParamStrings[route][i - numModelParams[model]]);
  }

  if (snow != SNOW_QTY) {
    int starti = numModelParams[model] + numRouteParams[route];
    int endi =
        numModelParams[model] + numRouteParams[route] + numSnowParams[snow];
    for (i = starti; i < endi; i++) {
      fprintf(file, ",%s\n", snowParamStrings[snow][i - starti]);
    }
  }

  fprintf(file, ",%s,%s/2%s", objectiveString, objectiveString, "\n");

  // Generate a 2D matrix with samples
  allocate2D(&ParSet, post_Sequences * pointerMCMC->seq, pointerMCMC->n + 2);
  GenParSet(ParSet, pointerRUNvar, post_Sequences, pointerMCMC, file,
            bestParams);

  //------Deallocating
  //Memory------------------------------------------------------//
  /*for (i = 0; i < floorf(1.25 * pointerRUNvar->Nelem); i++) {
   for (j=0; j < pointerMCMC->n+2; j++) {
   free(pointerRUNvar->Sequences[i][j]);
   }
   free(pointerRUNvar->Sequences[i]);
   }
   free(pointerRUNvar->Sequences);*/
  deallocate2D(&ParSet, post_Sequences * pointerMCMC->seq);
  // free(pointerRUNvar);
  fprintf(file, "[WaterBalance]\n");
  for (i = 0; i < numModelParams[model]; i++) {
    fprintf(file, "%s=%f\n", modelParamStrings[model][i], bestParams[i]);
  }
  fprintf(file, "[Routing]\n");
  endi = numModelParams[model] + numRouteParams[route];
  for (i = numModelParams[model]; i < endi; i++) {
    fprintf(file, "%s=%f\n",
            routeParamStrings[route][i - numModelParams[model]], bestParams[i]);
  }

  if (snow != SNOW_QTY) {
    fprintf(file, "[Snow]\n");
    int starti = numModelParams[model] + numRouteParams[route];
    int endi =
        numModelParams[model] + numRouteParams[route] + numSnowParams[snow];
    for (i = starti; i < endi; i++) {
      fprintf(file, "%s=%f\n", snowParamStrings[snow][i - starti],
              bestParams[i]);
    }
  }

  fclose(file);
}

void DREAM::CompDensity(float **p, float *log_p, float **x,
                        struct DREAM_Parameters *MCMC,
                        struct Model_Input *Input, int option) {
  // This function computes the density of each x value
  float objScore;
  int count = MCMC->seq;
  int i = 0;

  //#pragma omp parallel for private(objScore, i)
  if (!isEnsemble) {
#if _OPENMP
#pragma omp parallel for
#endif
    for (i = 0; i < count; i++) {
      float score = sim->SimulateForCali(&(x[i][0]));
      objScore = ((goal == OBJECTIVE_GOAL_MINIMIZE) ? -1.0 : 1.0f) * score;
#if _OPENMP
      printf("%i (%i) %f\n", i, omp_get_thread_num(), score);
#endif
      p[i][0] = objScore;
      p[i][1] = i;
      log_p[i] = 0.5 * objScore;
    }
  } else {
    // Ensemble calculate RHRE
    int numEns = (int)ensSims->size();
    int numObs = (int)ensSims->at(0).GetNumSteps();
    float *obsVals = ensSims->at(0).GetObsTS();
    float *dischargeVals[numEns];
    float currentDischargeSet[numEns];
    int bin_tally[numEns + 1];

    for (i = 0; i < count; i++) {
      int paramIndex = 0;

      for (int z = 0; z < numEns + 1; z++) {
        bin_tally[z] = 0;
      }

      for (int j = 0; j < numEns; j++) {
        dischargeVals[j] =
            ensSims->at(j).SimulateForCaliTS(&(x[i][paramIndex]));
        paramIndex += paramsPerSim->at(j);
      }

      for (int j = 0; j < numObs; j++) {
        for (int k = 0; k < numEns; k++) {
          currentDischargeSet[k] = -9999.0;
        }

        for (int k = 0; k < numEns; k++) {
          float cVal = dischargeVals[k][j];
          // printf("d%i: %f\n", k, cVal);
          for (int z = numEns - 1; z >= 0; z--) {
            // printf("test %f, %f, %i\n", cVal, currentDischargeSet[z], z);
            if (cVal >= currentDischargeSet[z]) {
              float tempVal = currentDischargeSet[z];
              currentDischargeSet[z] = cVal;
              // printf("dSet%i: %f\n", k, cVal);
              cVal = tempVal;
              if (cVal == -9999) {
                break;
              }
            }
          }
          // currentDischargeSet[k] = dischargeVals[k][j];
        }

        // printf("d: %f %f %f\n", currentDischargeSet[0],
        // currentDischargeSet[1], currentDischargeSet[2]);

        float obsVal = obsVals[j];
        int bin;
        for (bin = numEns - 1; bin >= 0; bin--) {
          if (obsVal >= currentDischargeSet[bin]) {
            break;
          }
        }
        bin_tally[bin + 1]++;
      }

      float distMax = bin_tally[0],
            distExpected = 1.0 / ((float)(numEns) + 1.0);
      for (int z = 0; z < numEns + 1; z++) {
        if (distMax < bin_tally[z]) {
          distMax = bin_tally[z];
        }
      }
      // printf("t: %i %i %i %i\n", bin_tally[0], bin_tally[1], bin_tally[2],
      // bin_tally[3]);
      distMax /= ((float)(numObs));
      objScore = fabs((distMax - distExpected) / distExpected) * 100.0;
      // printf("%f %f %f %f\n", distExpected, distMax, objScore,
      // (float)(numObs));
      p[i][0] = -1 * objScore;
      p[i][1] = i;
      log_p[i] = -0.5 * objScore;

      for (int j = 0; j < numEns; j++) {
        delete[] dischargeVals[j];
      }
    }

    delete[] obsVals;
  }
}
