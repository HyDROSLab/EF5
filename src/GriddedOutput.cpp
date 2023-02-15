#include "GriddedOutput.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

const char *GriddedOutputText[] = {
    "none",
    "streamflow",
    "soilmoisture",
    "returnperiod",
    "precip",
    "pet",
    "maxstreamflow",
    "maxsoilmoisture",
    "maxreturnperiod",
    "snowwater",
    "maxsnowwater",
    "temperature",
    "inundation",
    "unitstreamflow",
    "maxunitstreamflow",
    "thresholdexceedance",
    "maxthresholdexceedance",
    "maxthresholdexceedancep",
    "precipaccum",
    "runoff",
    "groundwater",
    "subrunoff",
};

const int GriddedOutputFlags[] = {
    0,
    1,
    2,
    4,
    8,
    16,
    32,
    64,
    128,
    256,
    512,
    1024,
    OG_DEPTH,
    OG_UNITQ,
    OG_MAXUNITQ,
    OG_THRES,
    OG_MAXTHRES,
    OG_MAXTHRESP,
    OG_PRECIPACCUM,
    OG_RUNOFF,
    OG_GW,
    OG_SUBSURF
};
