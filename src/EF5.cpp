#include <cstdio>
#include <unistd.h>

#include "Config.h"
#include "DEMProcessor.h"
#include "Defines.h"
#include "EF5.h"
#include "ExecutionController.h"

extern Config *g_config;

void PrintStartupMessage();

int main(int argc, char *argv[]) {

  PrintStartupMessage();

  if (argc <= 2) {

    g_config = new Config((argc == 2) ? argv[1] : "control.txt");
    if (g_config->ParseConfig() != CONFIG_SUCCESS) {
      return 1;
    }

    ExecuteTasks();
  } else {
    int opt = 0;
    int mode = 0;
    char *demFile = NULL, *flowDirFile = NULL, *flowAccFile = NULL;
    while ((opt = getopt(argc, argv, "z:d:a:ps")) != -1) {
      switch (opt) {
      case 'z':
        demFile = optarg;
        break;
      case 'd':
        flowDirFile = optarg;
        break;
      case 'a':
        flowAccFile = optarg;
        break;
      case 'p':
        mode = 1;
        break;
      case 's':
        mode = 2;
        break;
      }
    }
    ProcessDEM(mode, demFile, flowDirFile, flowAccFile);
  }

  return ERROR_SUCCESS;
}

void PrintStartupMessage() {
  printf("%s", "********************************************************\n");
  printf("%s", "**   Ensemble Framework For Flash Flood Forecasting   **\n");
  printf("**                   Version %s                     **\n",
         EF5_VERSION);
  printf("%s", "********************************************************\n");
}
