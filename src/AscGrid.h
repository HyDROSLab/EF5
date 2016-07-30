#ifndef ASC_GRID_H
#define ASC_GRID_H

#include "Grid.h"

LongGrid *ReadLongAscGrid(char *file);
FloatGrid *ReadFloatAscGrid(char *file);
void WriteLongAscGrid(const char *file, LongGrid *grid);
void WriteFloatAscGrid(const char *file, FloatGrid *grid);

#endif
