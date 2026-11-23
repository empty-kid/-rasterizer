#ifndef SMOOTHING_H
#define SMOOTHING_H

#include "vectors.h"
#include <vector>
#include <math.h>

void fxaa(std::vector<float> &fb, std::vector<float> &tb, int W, int H);
void fxaa_inter(std::vector<float> &fb, std::vector<float> &tb, std::vector<float> &lb, int W, int H);


#endif
