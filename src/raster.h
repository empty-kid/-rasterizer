#ifndef RASTER_H
#define RASTER_H

#include "mesh_utils.h"
#include "mat.h"
#include <vector>
#include <string>

struct ShadowMap {
    std::vector<float> data;
    int size = 0;
    mat4 VP;
    float bias = 0.0004f;
    float slope = 0.001f;
};

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const vec3 &l, const vec3 &k_a, const vec3 &k_d, const std::string &type, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const vec3 &l, const vec3 &k_a, const vec3 &k_d, const texture *tex, const std::string &type, bool back_sides, const ShadowMap &shadow);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const texture *tex, const std::string &type);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT_L (&verts)[3], const vec3 &l, const vec3 &k_a, const vec3 &k_d, const texture *tex, const texture &add_tex, const std::string &type, bool back_sides, const ShadowMap &shadow);

void rasterisation(float *data, int shadow_size, const VERT (&verts)[3]);

vec3 interpolate(const float* tex, int tw, int th, vec2 t, bool clamp);

#endif
