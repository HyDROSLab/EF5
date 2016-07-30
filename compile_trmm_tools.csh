#!/bin/csh

g++ -O3 -o bin/TRMMRTClip src/TRMMRTClip.cpp src/TRMMRTGrid.cpp src/AscGrid.cpp -lz
g++ -O3 -o bin/TRMMDClip src/TRMMDClip.cpp src/TRMMDGrid.cpp src/AscGrid.cpp -lz
g++ -O3 -o bin/TRMMV6Clip src/TRMMV6Clip.cpp src/TRMMV6Grid.cpp src/AscGrid.cpp /usr/lib64/hdf/libmfhdf.a /usr/lib64/hdf/libdf.a -lz -ljpeg
g++ -O3 -o bin/BIFClip src/BIFClip.cpp src/BifGrid.cpp src/AscGrid.cpp
g++ -O3 -o bin/MRMSConvert src/MRMSConvert.cpp src/TifGrid.cpp src/MRMSGrid.cpp -lz -ltiff -lgeotiff
g++ -g -O3 -o bin/MRMSSum src/MRMSSum.cpp src/TifGrid.cpp src/MRMSGrid.cpp -lz -ltiff -lgeotiff
g++ -g -O3 -o bin/ComputeRP src/ComputeRP.cpp src/TifGrid.cpp -lz -ltiff -lgeotiff
