#include <noise.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

float Perlin2d::lerp(float a, float b, float t)
{
    return a * (1 - t) + b * t;
}
float Perlin2d::QunticCurve(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

vec2 Perlin2d::GetPseudoRandomGradientVector(int x, int y)
{
    int v = (int)((((long long)x * 1836311903ll) ^ ((long long)y * 2971215073ll) + 4807526976ll) & 1023ll);
    v = permutationTable[v] & 3;

    switch (v)
    {
        case 0:  return(make2(  1, 0 ));
        case 1:  return(make2( -1, 0 ));
        case 2:  return(make2(  0, 1 ));
        default: return(make2(  0,-1 ));
    }
}

Perlin2d::Perlin2d(int seed)
{
    for (int i = 0; i < 256; i++) {
        permutationTable[i] = (unsigned short)i;
    }

    std::mt19937 rng(seed);
    std::shuffle(permutationTable, permutationTable + 256, rng);

    for (int i = 0; i < 256; i++) {
        permutationTable[i + 256] = permutationTable[i];
    }
    for (int i = 0; i < 512; i++) {
        permutationTable[i + 512] = permutationTable[i];
    }
}

float Perlin2d::noise(float x, float y)
{
    int left = (int)floorf(x);
    int top  = (int)floorf(y);
    float dx = x - left;
    float dy = y - top;

    vec2 topLeftGradient     = GetPseudoRandomGradientVector(left,   top  );
    vec2 topRightGradient    = GetPseudoRandomGradientVector(left+1, top  );
    vec2 bottomLeftGradient  = GetPseudoRandomGradientVector(left,   top+1);
    vec2 bottomRightGradient = GetPseudoRandomGradientVector(left+1, top+1);

    vec2 distanceToTopLeft     = make2( dx,   dy   );
    vec2 distanceToTopRight    = make2( dx-1, dy   );
    vec2 distanceToBottomLeft  = make2( dx,   dy-1 );
    vec2 distanceToBottomRight = make2( dx-1, dy-1 );

    float tx1 = dot(distanceToTopLeft,     topLeftGradient);
    float tx2 = dot(distanceToTopRight,    topRightGradient);
    float bx1 = dot(distanceToBottomLeft,  bottomLeftGradient);
    float bx2 = dot(distanceToBottomRight, bottomRightGradient);

    dx = QunticCurve(dx);
    dy = QunticCurve(dy);

    float tx = lerp(tx1, tx2, dx);
    float bx = lerp(bx1, bx2, dx);
    float tb = lerp(tx, bx, dy);

    return tb;
}

float Perlin2d::noise(float x, float y, int octaves, float persistence)
{
    float amplitude = 1;
    float max = 0;
    float result = 0;

    while (octaves-- > 0)
    {
        max += amplitude;
        result += noise(x, y) * amplitude;
        amplitude *= persistence;
        x *= 2;
        y *= 2;
    }

    return result/max;
}