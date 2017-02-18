AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

.rc.o:
	$(WINDRES) src/ef5.rc -o $@

bin_PROGRAMS = $(top_builddir)/bin/ef5
unit_FILES = src/LAEAProjection.cpp src/GeographicProjection.cpp src/DistanceUnit.cpp src/TimeUnit.cpp src/DistancePerTimeUnits.cpp src/TimeVar.cpp
type_FILES = src/DatedName.cpp src/PETType.cpp src/PrecipType.cpp src/TempType.cpp src/GaugeMap.cpp
config_FILES = src/BasicConfigSection.cpp src/PrecipConfigSection.cpp src/PETConfigSection.cpp src/TempConfigSection.cpp src/GaugeConfigSection.cpp src/BasinConfigSection.cpp src/CaliParamConfigSection.cpp src/ParamSetConfigSection.cpp src/RoutingCaliParamConfigSection.cpp src/RoutingParamSetConfigSection.cpp src/TaskConfigSection.cpp src/EnsTaskConfigSection.cpp src/ExecuteConfigSection.cpp src/Config.cpp src/SnowCaliParamConfigSection.cpp src/SnowParamSetConfigSection.cpp src/InundationCaliParamConfigSection.cpp src/InundationParamSetConfigSection.cpp
input_FILES = src/RPSkewness.cpp src/TimeSeries.cpp src/PETReader.cpp src/PrecipReader.cpp src/TempReader.cpp src/TifGrid.cpp src/BifGrid.cpp src/AscGrid.cpp src/BasicGrids.cpp src/TRMMRTGrid.cpp src/MRMSGrid.cpp src/GridWriter.cpp src/GridWriterFull.cpp src/GriddedOutput.cpp
model_FILES = src/Model.cpp src/CRESTModel.cpp src/HyMOD.cpp src/SAC.cpp src/LinearRoute.cpp src/KinematicRoute.cpp src/ObjectiveFunc.cpp src/Simulator.cpp src/ARS.cpp src/DREAM.cpp src/dream_functions.cpp src/misc_functions.cpp src/Snow17Model.cpp src/HPModel.cpp src/SimpleInundation.cpp src/VCInundation.cpp
if WINDOWS
AM_CXXFLAGS= -Wall -mwindows ${OPENMP_CFLAGS}
__top_builddir__bin_ef5_SOURCES = $(unit_FILES) $(type_FILES) $(config_FILES) $(input_FILES) $(model_FILES) src/ExecutionController.cpp src/EF5Windows.cpp src/ef5.rc
__top_builddir__bin_ef5_LDADD=-ltiff -lgeotiff -lz
else
AM_CXXFLAGS= -Wall -Werror ${OPENMP_CFLAGS}
__top_builddir__bin_ef5_SOURCES = $(unit_FILES) $(type_FILES) $(config_FILES) $(input_FILES) $(model_FILES) src/ExecutionController.cpp src/EF5.cpp src/DEMProcessor.cpp
__top_builddir__bin_ef5_LDADD=-ltiff -lgeotiff -lz -lgomp
endif

EXTRA_PROGRAMS = $(top_builddir)/bin/kwtest
__top_builddir__bin_kwtest_SOURCES = $(unit_FILES) $(type_FILES) $(config_FILES) $(input_FILES) $(model_FILES) src/KWTest.cpp
