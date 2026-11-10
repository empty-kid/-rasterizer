#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "vectors.h"
#include "mat.h"
#include "raster.h"
#include "mesh_utils.h"

int clip_near(const VERT (&in)[3], VERT (&out)[4]);

void paint(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &objs, int W, int H);

void paint_background(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &objs, int W, int H);

void paint_landscape(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, object &landscape, int W, int H, const std::vector<float> height_map, int hm_weight, int hm_height, int landscape_size, float landscape_base_y);

#endif