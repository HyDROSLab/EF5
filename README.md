Ensemble Framework For Flash Flood Forecasting (EF5)
===
![version](https://img.shields.io/badge/version-1.1-blue.svg?style=flat) [![Build Status](https://travis-ci.org/HyDROSLab/EF5.svg?branch=master)](https://travis-ci.org/HyDROSLab/EF5)

EF5 was created by the Hydrometeorology and Remote Sensing Laboratory at the University of Oklahoma.
The goal of EF5 is to have a framework for distributed hydrologic modeling that is user friendly, adaptable, expandable, all while being suitable for large scale (e.g. continental scale) modeling of flash floods with rapid forecast updates. Currently EF5 incorporates 3 water balance models including the Sacramento Soil Moisture Accouning Model (SAC-SMA), Coupled Routing and Excess Storage (CREST), and hydrophobic (HP). These water balance models can be coupled with either linear reservoir or kinematic wave routing. 

## Learn More

EF5 has a homepage at [http://ef5.ou.edu](http://ef5.ou.edu). The training modules are found at [http://ef5.ou.edu/training/](http://ef5.ou.edu/training/) while the YouTube videos may be found at [https://www.youtube.com/channel/UCgoGJtdeqHgwoYIRhkgMwog](https://www.youtube.com/channel/UCgoGJtdeqHgwoYIRhkgMwog). The source code is found on GitHub at [https://github.com/HyDROSLab/EF5](https://github.com/HyDROSLab/EF5).

See [manual.html](manual.html) for the EF5 operating manual which describes configuration options.

## Compiling

### Linux

Clone the source code from GitHub.   
1. autoreconf --force --install   
2. ./configure   
3. make   
   This compiles the EF5 application!

### OS X

Clone the source code from GitHub. Use the EF5 Xcode project found in the EF5 folder and compile the project.

### Windows

Currently cross-compiling from Linux is the recommended way of generating Windows binaries.

Clone the source code from GitHub.

1. autoreconf --force --install
2. For 32-bit Windows installations use ./configure --host=i686-w64-mingw32   
   For 64-bit Windows installations use ./configure --host=x86_64-w64-mingw32

3. make   
   This compiles the EF5 application!

## Contributors

The following people are acknowledged for their contributions to the creation of EF5.

Zac Flamig

Humberto Vergara

Race Clark

JJ Gourley

Yang Hong

