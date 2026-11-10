#ifndef NOISE_H
#define NOISE_H

#include <vectors.h>

class Perlin2d
{
public:
    Perlin2d(int seed);
    static float lerp(float a, float b, float t);
    static float QunticCurve(float t);
    float noise(float x, float y);
    float noise(float x, float y, int octaves, float persistence);

private:
    unsigned short permutationTable[1024];
    vec2 GetPseudoRandomGradientVector(int x, int y);
};

#endif