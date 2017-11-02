#ifndef MODEL_H
#define MODEL_H

#define ADDMODEL(a, b)
#define ADDPARAMCREST(a, b)
#define ADDPARAMHYMOD(a, b)
#define ADDPARAMSAC(a, b)
#define ADDPARAMHP(a, b)
#define ADDROUTE(a, b)
#define ADDPARAMLINEAR(a, b)
#define ADDPARAMKINEMATIC(a, b)
#define ADDSNOW(a, b)
#define ADDPARAMSNOW17(a, b)
#define ADDINUNDATION(a, b)
#define ADDPARAMSI(a, b)
#define ADDPARAMVCI(a, b)

enum RUNSTYLE {
  STYLE_SIMU,
  STYLE_SIMU_RP,
  STYLE_CALI_ARS,
  STYLE_CALI_DREAM,
  STYLE_CLIP_BASIN,
  STYLE_CLIP_GAUGE,
  STYLE_MAKE_BASIC,
  STYLE_BASIN_AVG,
  STYLE_QTY,
};

enum MODELS {
#undef ADDMODEL
#define ADDMODEL(a, b) MODEL_##b,
#include "Models.tbl"
#undef ADDMODEL
#define ADDMODEL(a, b)

  MODEL_QTY,
};

enum CREST_PARAMS {
#undef ADDPARAMCREST
#define ADDPARAMCREST(a, b) PARAM_CREST_##b,
#include "Models.tbl"
#undef ADDPARAMCREST
#define ADDPARAMCREST(a, b)

  PARAM_CREST_QTY,
};

enum HYMOD_PARAMS {
#undef ADDPARAMHYMOD
#define ADDPARAMHYMOD(a, b) PARAM_HYMOD_##b,
#include "Models.tbl"
#undef ADDPARAMHYMOD
#define ADDPARAMHYMOD(a, b)

  PARAM_HYMOD_QTY,
};

enum SAC_PARAMS {
#undef ADDPARAMSAC
#define ADDPARAMSAC(a, b) PARAM_SAC_##b,
#include "Models.tbl"
#undef ADDPARAMSAC
#define ADDPARAMSAC(a, b)

  PARAM_SAC_QTY,
};

enum HP_PARAMS {
#undef ADDPARAMHP
#define ADDPARAMHP(a, b) PARAM_HP_##b,
#include "Models.tbl"
#undef ADDPARAMHP
#define ADDPARAMHP(a, b)

  PARAM_HP_QTY,
};

enum ROUTES {
#undef ADDROUTE
#define ADDROUTE(a, b) ROUTE_##b,
#include "Models.tbl"
#undef ADDROUTE
#define ADDROUTE(a, b)

  ROUTE_QTY,
};

enum LINEAR_PARAMS {
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b) PARAM_LINEAR_##b,
#include "Models.tbl"
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b)

  PARAM_LINEAR_QTY,
};

enum KINEMATIC_PARAMS {
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b) PARAM_KINEMATIC_##b,
#include "Models.tbl"
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b)

  PARAM_KINEMATIC_QTY,
};

enum SNOWS {
#undef ADDSNOW
#define ADDSNOW(a, b) SNOW_##b,
#include "Models.tbl"
#undef ADDSNOW
#define ADDSNOW(a, b)

  SNOW_QTY,
};

enum SNOW17_PARAMS {
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b) PARAM_SNOW17_##b,
#include "Models.tbl"
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b)

  PARAM_SNOW17_QTY,
};

enum INUNDATIONS {
#undef ADDINUNDATION
#define ADDINUNDATION(a, b) INUNDATION_##b,
#include "Models.tbl"
#undef ADDINUNDATION
#define ADDINUNDATION(a, b)

  INUNDATION_QTY,
};

enum SI_PARAMS {
#undef ADDPARAMSI
#define ADDPARAMSI(a, b) PARAM_SI_##b,
#include "Models.tbl"
#undef ADDPARAMSI
#define ADDPARAMSI(a, b)

  PARAM_SI_QTY,
};

enum VCI_PARAMS {
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b) PARAM_VCI_##b,
#include "Models.tbl"
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b)

  PARAM_VCI_QTY,
};

extern const char *runStyleStrings[];
extern const char *modelStrings[];
extern const char *modelParamSetStrings[];
extern const char *modelCaliParamStrings[];
extern const char **modelParamStrings[];
extern const char **modelParamGridStrings[];
extern const int numModelParams[];

extern const char *routeStrings[];
extern const char *routeParamSetStrings[];
extern const char *routeCaliParamStrings[];
extern const char **routeParamStrings[];
extern const char **routeParamGridStrings[];
extern const int numRouteParams[];

extern const char *snowStrings[];
extern const char *snowParamSetStrings[];
extern const char *snowCaliParamStrings[];
extern const char **snowParamStrings[];
extern const char **snowParamGridStrings[];
extern const int numSnowParams[];

extern const char *inundationStrings[];
extern const char *inundationParamSetStrings[];
extern const char *inundationCaliParamStrings[];
extern const char **inundationParamStrings[];
extern const char **inundationParamGridStrings[];
extern const int numInundationParams[];

#define IsCalibrationRunStyle(style)                                           \
  ((style) == STYLE_CALI_ARS || (style) == STYLE_CALI_DREAM)

#endif
