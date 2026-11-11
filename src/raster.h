#ifndef RASTER_H
#define RASTER_H

#include "mesh_utils.h"
#include <vector>
#include <string>


void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const std::string &type, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const texture *tex, const std::string &type, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const texture *tex, const std::string &type);

void rasterisation(float *data, float *z_buf, int w, int h, const VERT_L (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const texture *tex, const texture &add_tex, const std::string &type, bool back_sides);

vec3 interpolate(const float* tex, int tw, int th, vec2 t, bool clamp);

#endif