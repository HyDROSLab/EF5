#ifndef TIF_GRID_H
#define TIF_GRID_H

#include "Grid.h"

FloatGrid *ReadFloatTifGrid(const char *file);
FloatGrid *ReadFloatTifGrid(const char *file, FloatGrid *incGrid);
void WriteFloatTifGrid(const char *file, FloatGrid *grid);
LongGrid *ReadLongTifGrid(const char *file);

#endif
