#include "RPSkewness.h"
#include "AscGrid.h"
#include "BasicGrids.h"
#include "GridWriter.h"
#include <cmath>
#include <cstdio>

float cs[] = {
    -3.0, -2.9, -2.8, -2.7, -2.6, -2.5, -2.4, -2.3, -2.2, -2.1, -2.0,
    -1.9, -1.8, -1.7, -1.6, -1.5, -1.4, -1.3, -1.2, -1.1, -1.0, -0.9,
    -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0.0,  0.1,  0.2,
    0.3,  0.4,  0.5,  0.6,  0.7,  0.8,  0.9,  1.0,  1.1,  1.2,  1.3,
    1.4,  1.5,  1.6,  1.7,  1.8,  1.9,  2.0,  2.1,  2.2,  2.3,  2.4,
    2.5,  2.6,  2.7,  2.8,  2.9,  3.0,
};

float lut1[] = {
    -4.051, -4.013, -3.973, -3.932, -3.899, -3.845, -3.8,   -3.753, -3.705,
    -3.656, -3.605, -3.553, -3.499, -3.444, -3.88,  -3.33,  -3.271, -3.211,
    -3.149, -3.087, -3.022, -2.957, -2.891, -2.824, -2.755, -2.686, -2.615,
    -2.544, -2.472, -2.4,   -2.326, -2.252, -2.178, -2.104, -2.029, -1.955,
    -1.88,  -1.806, -1.733, -1.66,  -1.588, -1.518, -1.449, -1.383, -1.256,
    -1.197, -1.14,  -1.087, -1.037, -0.99,  -0.946, -0.905, -0.867, -0.832,
    -0.799, -0.769, -0.74,  -0.714, -0.69,  -0.667,
};

float lut2[] = {
    0.396,  0.39,   0.384,  0.376,  0.368,  0.36,   0.351,  0.341,  0.33,
    0.319,  0.307,  0.294,  0.282,  0.268,  0.254,  0.24,   0.225,  0.21,
    0.195,  0.18,   0.164,  0.148,  0.132,  0.116,  0.099,  0.083,  0.066,
    0.05,   0.033,  0.017,  0.0,    -0.017, -0.033, -0.05,  -0.066, -0.083,
    -0.099, -0.116, -0.132, -0.148, -0.164, -0.18,  -0.195, -0.21,  -0.225,
    -0.24,  -0.254, -0.268, -0.282, -0.294, -0.307, -0.319, -0.33,  -0.341,
    -0.351, -0.36,  -0.368, -0.376, -0.384, -0.39,  -0.396,
};

float lut5[] = {
    0.636, 0.651, 0.666, 0.681, 0.696, 0.711, 0.725, 0.739, 0.752, 0.765, 0.777,
    0.788, 0.799, 0.808, 0.817, 0.825, 0.832, 0.838, 0.844, 0.848, 0.852, 0.854,
    0.856, 0.857, 0.857, 0.856, 0.855, 0.853, 0.85,  0.846, 0.842, 0.836, 0.83,
    0.824, 0.816, 0.808, 0.8,   0.79,  0.78,  0.769, 0.758, 0.745, 0.732, 0.719,
    0.705, 0.69,  0.675, 0.66,  0.643, 0.627, 0.609, 0.592, 0.574, 0.555, 0.537,
    0.518, 0.499, 0.479, 0.46,  0.44,  0.42,
};

float lut10[] = {
    0.66,  0.681, 0.702, 0.724, 0.747, 0.711, 0.795, 0.819, 0.844, 0.869, 0.895,
    0.92,  0.945, 0.97,  0.994, 1.018, 1.041, 1.064, 1.086, 1.107, 1.128, 1.147,
    1.166, 1.183, 1.2,   1.216, 1.231, 1.245, 1.258, 1.27,  1.282, 1.292, 1.301,
    1.309, 1.317, 1.323, 1.328, 1.333, 1.336, 1.339, 1.34,  1.341, 1.34,  1.339,
    1.337, 1.333, 1.329, 1.324, 1.318, 1.31,  1.302, 1.294, 1.284, 1.274, 1.262,
    1.25,  1.238, 1.224, 1.21,  1.195, 1.18,
};

float lut25[] = {
    0.666, 0.683, 0.712, 0.738, 0.764, 0.793, 0.823, 0.855, 0.888, 0.923, 0.959,
    0.996, 1.035, 1.075, 1.116, 1.157, 1.198, 1.24,  1.282, 1.324, 1.366, 1.407,
    1.448, 1.488, 1.528, 1.567, 1.606, 1.643, 1.68,  1.716, 1.751, 1.785, 1.818,
    1.849, 1.88,  1.91,  1.939, 1.967, 1.993, 2.018, 2.043, 2.066, 2.087, 2.108,
    2.128, 2.146, 2.163, 2.179, 2.193, 2.207, 2.219, 2.23,  2.24,  2.248, 2.256,
    2.262, 2.267, 2.272, 2.275, 2.277, 2.278,
};

float lut50[] = {
    0.666, 0.689, 0.714, 0.74,  0.768, 0.798, 0.83,  0.864, 0.9,   0.939, 0.98,
    1.023, 1.069, 1.116, 1.166, 1.217, 1.27,  1.324, 1.379, 1.435, 1.492, 1.549,
    1.606, 1.663, 1.72,  1.777, 1.834, 1.89,  1.945, 2,     2.054, 2.107, 2.159,
    2.211, 2.261, 2.311, 2.359, 2.407, 2.453, 2.498, 2.542, 2.585, 2.626, 2.666,
    2.706, 2.743, 2.78,  2.815, 2.848, 2.881, 2.912, 2.942, 2.97,  2.997, 3.023,
    3.048, 3.071, 3.093, 3.114, 3.134, 3.152,
};

float lut100[] = {
    0.667, 0.69,  0.714, 0.74,  0.769, 0.799, 0.832, 0.867, 0.905, 0.946, 0.99,
    1.037, 1.087, 1.14,  1.197, 1.256, 1.318, 1.383, 1.449, 1.518, 1.588, 1.66,
    1.733, 1.806, 1.88,  1.955, 2.029, 2.104, 2.178, 2.252, 2.326, 2.4,   2.472,
    2.544, 2.615, 2.686, 2.755, 2.824, 2.891, 2.957, 3.022, 3.087, 3.149, 3.211,
    3.271, 3.33,  3.388, 3.444, 3.499, 3.553, 3.605, 3.656, 3.705, 3.753, 3.8,
    3.845, 3.889, 3.932, 3.973, 4.013, 4.051,
};

float lut200[] = {
    0.667, 0.69,  0.714, 0.741, 0.769, 0.8,   0.833, 0.869, 0.907, 0.949, 0.995,
    1.044, 1.097, 1.155, 1.216, 1.282, 1.351, 1.424, 1.501, 1.581, 1.664, 1.749,
    1.837, 1.926, 2.016, 2.108, 2.201, 2.294, 2.388, 2.482, 2.576, 2.67,  2.763,
    2.856, 2.949, 3.041, 3.132, 3.223, 3.312, 3.401, 3.489, 3.575, 3.661, 3.745,
    3.828, 3.91,  3.99,  4.069, 4.147, 4.223, 4.298, 4.372, 4.444, 4.515, 4.584,
    4.652, 4.718, 4.783, 4.847, 4.904, 4.97,
};

bool ReadLP3File(char *file, std::vector<GridNode> *nodes,
                 std::vector<float> *lp3Vals) {
  FloatGrid *grid = NULL;

  grid = ReadFloatTifGrid(file);

  if (!grid) {
    return false;
  }

  // We have two options now... Either the grid & the basic grids are the same
  // Or they are different!

  if (g_DEM->IsSpatialMatch(grid)) {
    printf("Loading exact match LP3 grid %s\n", file);
    // The grids are the same! Our life is easy!
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (grid->data[node->y][node->x] != grid->noData) {
        lp3Vals->at(i) = grid->data[node->y][node->x];
      } else {
        lp3Vals->at(i) = 0;
      }
    }

  } else {
    printf("LP3 grids aren't an exact match so guessing! %s\n", file);
    // The grids are different, we must do some resampling fun.
    GridLoc pt;
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (grid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
          grid->data[pt.y][pt.x] != grid->noData) {
        lp3Vals->at(i) = grid->data[pt.y][pt.x];
      } else {
        lp3Vals->at(i) = 0;
      }
    }
  }

  delete grid;

  return true;
}

void CalcLP3Vals(std::vector<float> *stdGrid, std::vector<float> *avgGrid,
                 std::vector<float> *scGrid, std::vector<RPData> *rpData,
                 std::vector<GridNode> *nodes) {
  std::vector<float> count2;
  count2.resize(rpData->size());
  for (size_t i = 0; i < rpData->size(); i++) {
    int index = (int)(scGrid->at(i) * 10.0 + 30.0);
    if (index < 0) {
      index = 0;
      /*rpData->at(i).q1 = -1;
      rpData->at(i).q2 = -1;
      rpData->at(i).q5 = -1;
      rpData->at(i).q10 = -1;
      rpData->at(i).q25 = -1;
      rpData->at(i).q50 = -1;
      rpData->at(i).q100 = -1;
      rpData->at(i).q200 = -1;
      continue;*/
    } else if (index >= 60) {
      index = 59;
    }

    // 1
    float diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    float diff_q = lut1[index + 1] - lut1[index];
    float lerp = lut1[index] + diff_cs * diff_q;
    float std = stdGrid->at(i);
    float avg = avgGrid->at(i);
    rpData->at(i).q1 = powf(10.0, avg + lerp * std);
    // 2
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut2[index + 1] - lut2[index];
    lerp = lut2[index] + diff_cs * diff_q;
    rpData->at(i).q2 = powf(10.0, avg + lerp * std);
    count2[i] = rpData->at(i).q2;
    // 5
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut5[index + 1] - lut5[index];
    lerp = lut5[index] + diff_cs * diff_q;
    rpData->at(i).q5 = powf(10.0, avg + lerp * std);
    // 10
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut10[index + 1] - lut10[index];
    lerp = lut10[index] + diff_cs * diff_q;
    rpData->at(i).q10 = powf(10.0, avg + lerp * std);
    // 25
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut25[index + 1] - lut25[index];
    lerp = lut25[index] + diff_cs * diff_q;
    rpData->at(i).q25 = powf(10.0, avg + lerp * std);
    // 50
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut50[index + 1] - lut50[index];
    lerp = lut50[index] + diff_cs * diff_q;
    rpData->at(i).q50 = powf(10.0, avg + lerp * std);
    // 100
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut100[index + 1] - lut100[index];
    lerp = lut100[index] + diff_cs * diff_q;
    rpData->at(i).q100 = powf(10.0, avg + lerp * std);
    // 200
    diff_cs = (scGrid->at(i) - cs[index]) / (cs[index + 1] - cs[index]);
    diff_q = lut200[index + 1] - lut200[index];
    lerp = lut200[index] + diff_cs * diff_q;
    rpData->at(i).q200 = powf(10.0, avg + lerp * std);
  }
  // GridWriter gridWriter;
  // gridWriter.Initialize(nodes);
  // gridWriter.WriteGrid(nodes, &count2, "2qflow.asc");
}
#ifdef LOGNORMAL
void CalcLP3Vals(std::vector<float> *stdGrid, std::vector<float> *avgGrid,
                 std::vector<float> *scGrid, std::vector<RPData> *rpData,
                 std::vector<GridNode> *nodes) {
  std::vector<float> count2;
  count2.resize(rpData->size());
  for (size_t i = 0; i < rpData->size(); i++) {

    float lerp = -2.326;
    rpData->at(i).q1 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 2
    lerp = 0.000;
    rpData->at(i).q2 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    count2[i] = rpData->at(i).q2;
    // 5
    lerp = 0.842;
    rpData->at(i).q5 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 10
    lerp = 1.282;
    rpData->at(i).q10 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 25
    lerp = 1.751;
    rpData->at(i).q25 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 50
    lerp = 2.054;
    rpData->at(i).q50 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 100
    lerp = 2.326;
    rpData->at(i).q100 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
    // 200
    lerp = 2.576;
    rpData->at(i).q200 = powf(10.0, avgGrid->at(i) + lerp * stdGrid->at(i));
  }
  GridWriter gridWriter;
  gridWriter.Initialize(nodes);
  gridWriter.WriteGrid(nodes, &count2, "2qflow.asc");
}
#endif
float GetReturnPeriod(float q, RPData *rpData) {
  if (rpData->q1 == -1) {
    return -1;
  }

  if (q > rpData->q200) {
    return 200; // No Extrapolation...
  }

  float diff_q, diff_rp;

  if (q > rpData->q100) {
    diff_q = (q - rpData->q100) / (rpData->q200 - rpData->q100);
    diff_rp = 200.0 - 100.0;
    return 100.0 + diff_q * diff_rp;
  }

  if (q > rpData->q50) {
    diff_q = (q - rpData->q50) / (rpData->q100 - rpData->q50);
    diff_rp = 100.0 - 50.0;
    return 50.0 + diff_q * diff_rp;
  }

  if (q > rpData->q25) {
    diff_q = (q - rpData->q25) / (rpData->q50 - rpData->q25);
    diff_rp = 50.0 - 25.0;
    return 25.0 + diff_q * diff_rp;
  }

  if (q > rpData->q10) {
    diff_q = (q - rpData->q10) / (rpData->q25 - rpData->q10);
    diff_rp = 25.0 - 10.0;
    return 10.0 + diff_q * diff_rp;
  }

  if (q > rpData->q5) {
    diff_q = (q - rpData->q5) / (rpData->q10 - rpData->q5);
    diff_rp = 10.0 - 5.0;
    return 5.0 + diff_q * diff_rp;
  }

  if (q > rpData->q2) {
    diff_q = (q - rpData->q2) / (rpData->q5 - rpData->q2);
    diff_rp = 5.0 - 2.0;
    return 2.0 + diff_q * diff_rp;
  }

  if (q > rpData->q1) {
    diff_q = (q - rpData->q1) / (rpData->q2 - rpData->q1);
    diff_rp = 2.0 - 1.0;
    float retVal = 1.0 + diff_q * diff_rp;
    if (retVal >= 2.0) {
      retVal = 1.99;
    }
    return retVal;
  }

  return 0;
}
