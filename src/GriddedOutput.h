#ifndef GRIDDEDOUTPUT_H
#define GRIDDEDOUTPUT_H

enum SUPPORTED_OUTPUT_GRIDS {
  OG_NONE = 0,
  OG_Q = 1,
  OG_SM = 2,
  OG_QRP = 4,
  OG_PRECIP = 8,
  OG_PET = 16,
  OG_MAXQ = 32,
  OG_MAXSM = 64,
  OG_MAXQRP = 128,
  OG_SWE = 256,
  OG_MAXSWE = 512,
  OG_TEMP = 1024,
  OG_DEPTH = 2048,
  OG_UNITQ = 4096,
  OG_MAXUNITQ = 8192,
  OG_THRES = 16384,
  OG_MAXTHRES = 32768,
  OG_MAXTHRESP = 65536,
  OG_PRECIPACCUM = 131072,
  OG_RUNOFF=262144, //surface runoff (overland)
  OG_GW=524288, //Groundwater state
  OG_SUBSURF=1048576, //subsurface runoff
};

#define OG_QTY 22

extern const char *GriddedOutputText[];
extern const int GriddedOutputFlags[];

#endif
