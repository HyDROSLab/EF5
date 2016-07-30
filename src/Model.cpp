#include "Model.h"

const char *runStyleStrings[] = {
	"simu",
	"simu_rp",
	"cali_ars",
	"cali_dream",
	"clip_basin",
	"clip_gauge",
	"make_basic",
	"basin_avg",
};

const char *modelStrings[] = {
	#undef ADDMODEL
	#define ADDMODEL(a, b) a,
        #include "Models.tbl"
        #undef ADDMODEL
	#define ADDMODEL(a, b)
};

const char *modelParamSetStrings[] = {
        #undef ADDMODEL
        #define ADDMODEL(a, b) a"paramset",
        #include "Models.tbl"
        #undef ADDMODEL
        #define ADDMODEL(a, b)
};

const char *modelCaliParamStrings[] = {
        #undef ADDMODEL
        #define ADDMODEL(a, b) a"caliparams",
        #include "Models.tbl"
        #undef ADDMODEL
        #define ADDMODEL(a, b)
};

// Order of models must match that in Models.tbl

const char **modelParamStrings[] = {
	(const char *[]){
                #undef ADDPARAMHP
                #define ADDPARAMHP(a, b) a,
                #include "Models.tbl"
                #undef ADDPARAMHP
                #define ADDPARAMHP(a, b)

        },

	(const char *[]){
		#undef ADDPARAMCREST
		#define ADDPARAMCREST(a, b) a,
		#include "Models.tbl"
		#undef ADDPARAMCREST
		#define ADDPARAMCREST(a, b)
	
	},

	(const char *[]){
                #undef ADDPARAMHYMOD
                #define ADDPARAMHYMOD(a, b) a,
                #include "Models.tbl"
                #undef ADDPARAMHYMOD
                #define ADDPARAMHYMOD(a, b)

        },

	(const char *[]){
                #undef ADDPARAMSAC
                #define ADDPARAMSAC(a, b) a,
                #include "Models.tbl"
                #undef ADDPARAMSAC
                #define ADDPARAMSAC(a, b)

        },

};

// Order of models must match that in Models.tbl

const char **modelParamGridStrings[] = {
	(const char *[]){
    #undef ADDPARAMHP
    #define ADDPARAMHP(a, b) a"_grid",
    #include "Models.tbl"
    #undef ADDPARAMHP
    #define ADDPARAMHP(a, b)
  },

        (const char *[]){
		#undef ADDPARAMCREST
		#define ADDPARAMCREST(a, b) a"_grid",
		#include "Models.tbl"
		#undef ADDPARAMCREST
		#define ADDPARAMCREST(a, b)

	},

	(const char *[]){
		#undef ADDPARAMHYMOD
		#define ADDPARAMHYMOD(a, b) a"_grid",
		#include "Models.tbl"
		#undef ADDPARAMHYMOD
		#define ADDPARAMHYMOD(a, b)

	},

	(const char *[]){
		#undef ADDPARAMSAC
		#define ADDPARAMSAC(a, b) a"_grid",
		#include "Models.tbl"
		#undef ADDPARAMSAC
		#define ADDPARAMSAC(a, b)
	},

};


const int numModelParams[] = {
	#undef ADDMODEL
        #define ADDMODEL(a, b) PARAM_ ##b ##_QTY,
        #include "Models.tbl"
        #undef ADDMODEL
        #define ADDMODEL(a, b)
};



const char *routeStrings[] = {
#undef ADDROUTE
#define ADDROUTE(a, b) a,
#include "Models.tbl"
#undef ADDROUTE
#define ADDROUTE(a, b)
};

const char *routeParamSetStrings[] = {
#undef ADDROUTE
#define ADDROUTE(a, b) a"paramset",
#include "Models.tbl"
#undef ADDROUTE
#define ADDROUTE(a, b)
};

const char *routeCaliParamStrings[] = {
#undef ADDROUTE
#define ADDROUTE(a, b) a"caliparams",
#include "Models.tbl"
#undef ADDROUTE
#define ADDROUTE(a, b)
};

const char **routeParamStrings[] = {
	(const char *[]){
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b) a,
#include "Models.tbl"
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b)
    
	},
  
	(const char *[]){
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b) a,
#include "Models.tbl"
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b)
    
  },

};

const char **routeParamGridStrings[] = {
  (const char *[]){
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b) a"_grid",
#include "Models.tbl"
#undef ADDPARAMLINEAR
#define ADDPARAMLINEAR(a, b)
    
	},
  
	(const char *[]){
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b) a"_grid",
#include "Models.tbl"
#undef ADDPARAMKINEMATIC
#define ADDPARAMKINEMATIC(a, b)
    
	},
};

const int numRouteParams[] = {
#undef ADDROUTE
#define ADDROUTE(a, b) PARAM_ ##b ##_QTY,
#include "Models.tbl"
#undef ADDROUTE
#define ADDROUTE(a, b)
};




const char *snowStrings[] = {
#undef ADDSNOW
#define ADDSNOW(a, b) a,
#include "Models.tbl"
#undef ADDSNOW
#define ADDSNOW(a, b)
};

const char *snowParamSetStrings[] = {
#undef ADDSNOW
#define ADDSNOW(a, b) a"paramset",
#include "Models.tbl"
#undef ADDSNOW
#define ADDSNOW(a, b)
};

const char *snowCaliParamStrings[] = {
#undef ADDSNOW
#define ADDSNOW(a, b) a"caliparams",
#include "Models.tbl"
#undef ADDSNOW
#define ADDSNOW(a, b)
};

const char **snowParamStrings[] = {
  (const char *[]){
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b) a,
#include "Models.tbl"
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b)
    
  },
};

const char **snowParamGridStrings[] = {
  (const char *[]){
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b) a"_grid",
#include "Models.tbl"
#undef ADDPARAMSNOW17
#define ADDPARAMSNOW17(a, b)
    
  },
  
};

const int numSnowParams[] = {
#undef ADDSNOW
#define ADDSNOW(a, b) PARAM_ ##b ##_QTY,
#include "Models.tbl"
#undef ADDSNOW
#define ADDSNOW(a, b)
};

const char *inundationStrings[] = {
#undef ADDINUNDATION
#define ADDINUNDATION(a, b) a,
#include "Models.tbl"
#undef ADDINUNDATION
#define ADDINUNDATION(a, b)
};

const char *inundationParamSetStrings[] = {
#undef ADDINUNDATION
#define ADDINUNDATION(a, b) a"paramset",
#include "Models.tbl"
#undef ADDINUNDATION
#define ADDINUNDATION(a, b)
};

const char *inundationCaliParamStrings[] = {
#undef ADDINUNDATION
#define ADDINUNDATION(a, b) a"caliparams",
#include "Models.tbl"
#undef ADDINUNDATION
#define ADDINUNDATION(a, b)
};

const char **inundationParamStrings[] = {
  (const char *[]){
#undef ADDPARAMSI
#define ADDPARAMSI(a, b) a,
#include "Models.tbl"
#undef ADDPARAMSI
#define ADDPARAMSI(a, b)

  },

	(const char *[]){
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b) a,
#include "Models.tbl"
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b)

  },
};

const char **inundationParamGridStrings[] = {
  (const char *[]){
#undef ADDPARAMSI
#define ADDPARAMSI(a, b) a"_grid",
#include "Models.tbl"
#undef ADDPARAMSI
#define ADDPARAMSI(a, b)

  },

	(const char *[]){
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b) a"_grid",
#include "Models.tbl"
#undef ADDPARAMVCI
#define ADDPARAMVCI(a, b)

  },

};

const int numInundationParams[] = {
#undef ADDINUNDATION
#define ADDINUNDATION(a, b) PARAM_ ##b ##_QTY,
#include "Models.tbl"
#undef ADDINUNDATION
#define ADDINUNDATION(a, b)
};
