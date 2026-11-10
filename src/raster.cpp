#include <mesh_utils.h>
#include <vector>
#include <cmath>

static const vec3 mip_colors[] = {
    make3(0.0f, 0.3f, 1.0f),
    make3(0.0f, 1.0f, 1.0f),
    make3(0.0f, 1.0f, 0.0f),
    make3(1.0f, 1.0f, 0.0f),
    make3(1.0f, 0.5f, 0.0f),
    make3(1.0f, 0.0f, 0.0f),
};

static float max(float a, float b) {
    return a > b ? a : b;
}

static float min(float a, float b) {
    return a < b ? a : b;
}

template <typename V>
static float cross_p(V p, vec4 v1, vec4 v2) 
{
    return (v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x);
}

vec3 interpolate(const float* tex, int tw, int th, vec2 t) 
{
    t.x = fminf(fmaxf(t.x, 0.0f), 1.0f);
    t.y = fminf(fmaxf(t.y, 0.0f), 1.0f);
    
    // if (t.x > 1) t.x = 1;
    // else if(t.x < -1) t.x = 0;
    // if (t.y > 1) t.y = 1;
    // else if(t.y < -1) t.y = 0;

    t.y = 1.0f - t.y;
    float x = t.x * tw - 0.5f;
    float y = t.y * th - 0.5f;
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    float alpha = x - x0;
    float beta = y - y0;

    x0 = x0 >= 0 ? x0 : tw - 1;
    int x1 = x0 + 1 < tw ? x0 + 1 : x0 + 1 - tw;
    y0 = y0 >= 0 ? y0 : th - 1;
    int y1 = y0 + 1 < th ? y0 + 1 : y0 + 1 - th;

    vec3 c;
    for (int i = 0; i < 3; i++) {
        float c00 = tex[(y0 * tw + x0) * 3 + i];
        float c10 = tex[(y0 * tw + x1) * 3 + i];
        float c01 = tex[(y1 * tw + x0) * 3 + i];
        float c11 = tex[(y1 * tw + x1) * 3 + i];
        c.M[i] = (c00 * (1 - alpha) + c10 * alpha) * (1 - beta) + (c01 * (1 - alpha) + c11 * alpha) * beta;
    }
    return c;
}

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const std::string &type, bool back_sides)
{
    float s = cross_p(verts[2].coord, verts[0].coord, verts[1].coord);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(verts[0].coord.x, max(verts[1].coord.x, verts[2].coord.x));
    int y_max = (int)max(verts[0].coord.y, max(verts[1].coord.y, verts[2].coord.y));
    int x_min = (int)min(verts[0].coord.x, min(verts[1].coord.x, verts[2].coord.x));
    int y_min = (int)min(verts[0].coord.y, min(verts[1].coord.y, verts[2].coord.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, verts[0].coord, verts[1].coord) / s;
            float e_23 = cross_p(v_center, verts[1].coord, verts[2].coord) / s;
            float e_31 = cross_p(v_center, verts[2].coord, verts[0].coord) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = verts[0].coord.z * e_23 + verts[1].coord.z * e_31 + verts[2].coord.z * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                if (type == "normals") {
                    vec3 n;
                    n.x = verts[0].normals.x * e_23 + verts[1].normals.x * e_31 + verts[2].normals.x * e_12;
                    n.y = verts[0].normals.y * e_23 + verts[1].normals.y * e_31 + verts[2].normals.y * e_12;
                    n.z = verts[0].normals.z * e_23 + verts[1].normals.z * e_31 + verts[2].normals.z * e_12;
                    normalize(n);
                    data[(y * w + x) * 3 + 0] = (n.x + 1) / 2;
                    data[(y * w + x) * 3 + 1] = (n.y + 1) / 2;
                    data[(y * w + x) * 3 + 2] = (n.z + 1) / 2;
                } else if (type == "uv") {
                    float iw = e_23 * verts[0].coord.w + e_31 * verts[1].coord.w + e_12 * verts[2].coord.w;
                    float u  = (e_23 * verts[0].uv.x + e_31 * verts[1].uv.x + e_12 * verts[2].uv.x) / iw;
                    float v  = (e_23 * verts[0].uv.y + e_31 * verts[1].uv.y + e_12 * verts[2].uv.y) / iw;
                    // u -= floorf(u);
                    // v -= floorf(v);
                    u = fminf(fmaxf(u, 0.0f), 1.0f);
                    v = fminf(fmaxf(v, 0.0f), 1.0f);
                    data[(y * w + x) * 3 + 0] = u;
                    data[(y * w + x) * 3 + 1] = v;
                    data[(y * w + x) * 3 + 2] = 0;
                }
            }  
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const texture *tex, const std::string &type, bool back_sides)
{
    float s = cross_p(verts[2].coord, verts[0].coord, verts[1].coord);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(verts[0].coord.x, max(verts[1].coord.x, verts[2].coord.x));
    int y_max = (int)max(verts[0].coord.y, max(verts[1].coord.y, verts[2].coord.y));
    int x_min = (int)min(verts[0].coord.x, min(verts[1].coord.x, verts[2].coord.x));
    int y_min = (int)min(verts[0].coord.y, min(verts[1].coord.y, verts[2].coord.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, verts[0].coord, verts[1].coord) / s;
            float e_23 = cross_p(v_center, verts[1].coord, verts[2].coord) / s;
            float e_31 = cross_p(v_center, verts[2].coord, verts[0].coord) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = verts[0].coord.z * e_23 + verts[1].coord.z * e_31 + verts[2].coord.z * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                float iw = e_23 * verts[0].coord.w + e_31 * verts[1].coord.w + e_12 * verts[2].coord.w;
                float u  = (e_23 * verts[0].uv.x + e_31 * verts[1].uv.x + e_12 * verts[2].uv.x) / iw;
                float v  = (e_23 * verts[0].uv.y + e_31 * verts[1].uv.y + e_12 * verts[2].uv.y) / iw;
                float dist = (2.0f * 0.1f * 100.0f) / (100.0f + 0.1f - z * (100.0f - 0.1f));
                float level = log2f(dist / 1.5f);
                if (level < 0) level = 0;
                if (ceilf(level) >= tex->cop_w.size()) level = tex->cop_w.size() - 1;
                if (type == "mip") {
                    int idx = floorf(level) < 6 ? floorf(level) : 5;
                    vec3 c = mip_colors[idx];
                    data[(y * w + x) * 3 + 0] = c.x;
                    data[(y * w + x) * 3 + 1] = c.y;
                    data[(y * w + x) * 3 + 2] = c.z;
                    continue;
                }
                vec3 tc;
                if (level != floorf(level)) {
                    vec3 tc1 = interpolate(tex->copies[(size_t)floorf(level)], tex->cop_w[(size_t)floorf(level)], tex->cop_h[(size_t)floorf(level)], make2(u, v));
                    vec3 tc2 = interpolate(tex->copies[(size_t)floorf(level) + 1], tex->cop_w[(size_t)floorf(level) + 1], tex->cop_h[(size_t)floorf(level) + 1], make2(u, v));
                    float k = 1 - (level - floorf(level));
                    tc = make3(tc1.x * k + tc2.x * (1 - k), tc1.y * k + tc2.y * (1 - k), tc1.z * k + tc2.z * (1 - k));
                } else {
                    tc = interpolate(tex->copies[(size_t)level], tex->cop_w[(size_t)level], tex->cop_h[(size_t)level], make2(u, v));
                }
                vec3 n;
                n.x = verts[0].normals.x * e_23 + verts[1].normals.x * e_31 + verts[2].normals.x * e_12;
                n.y = verts[0].normals.y * e_23 + verts[1].normals.y * e_31 + verts[2].normals.y * e_12;
                n.z = verts[0].normals.z * e_23 + verts[1].normals.z * e_31 + verts[2].normals.z * e_12;
                normalize(n);
                float nl = dot(n, l) > 0 ? dot(n, l) : 0;
                data[(y * w + x) * 3 + 0] = tc.x * (k_a.x + k_d.x * nl);
                data[(y * w + x) * 3 + 1] = tc.y * (k_a.y + k_d.y * nl);
                data[(y * w + x) * 3 + 2] = tc.z * (k_a.z + k_d.z * nl);
            }
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const texture *tex, const std::string &type) {
    float s = cross_p(verts[2].coord, verts[0].coord, verts[1].coord);
    if (fabs(s) < 1e-6f) {
        return;
    }
    int x_max = (int)max(verts[0].coord.x, max(verts[1].coord.x, verts[2].coord.x));
    int y_max = (int)max(verts[0].coord.y, max(verts[1].coord.y, verts[2].coord.y));
    int x_min = (int)min(verts[0].coord.x, min(verts[1].coord.x, verts[2].coord.x));
    int y_min = (int)min(verts[0].coord.y, min(verts[1].coord.y, verts[2].coord.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, verts[0].coord, verts[1].coord) / s;
            float e_23 = cross_p(v_center, verts[1].coord, verts[2].coord) / s;
            float e_31 = cross_p(v_center, verts[2].coord, verts[0].coord) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = verts[0].coord.z * e_23 + verts[1].coord.z * e_31 + verts[2].coord.z * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                if (type == "normals") {
                    vec3 n;
                    n.x = verts[0].normals.x * e_23 + verts[1].normals.x * e_31 + verts[2].normals.x * e_12;
                    n.y = verts[0].normals.y * e_23 + verts[1].normals.y * e_31 + verts[2].normals.y * e_12;
                    n.z = verts[0].normals.z * e_23 + verts[1].normals.z * e_31 + verts[2].normals.z * e_12;
                    normalize(n);
                    data[(y * w + x) * 3 + 0] = (n.x + 1) / 2;
                    data[(y * w + x) * 3 + 1] = (n.y + 1) / 2;
                    data[(y * w + x) * 3 + 2] = (n.z + 1) / 2;
                } else if (type == "uv") {
                    float iw = e_23 * verts[0].coord.w + e_31 * verts[1].coord.w + e_12 * verts[2].coord.w;
                    float u  = (e_23 * verts[0].uv.x + e_31 * verts[1].uv.x + e_12 * verts[2].uv.x) / iw;
                    float v  = (e_23 * verts[0].uv.y + e_31 * verts[1].uv.y + e_12 * verts[2].uv.y) / iw;
                    // u -= floorf(u);
                    // v -= floorf(v);
                    u = fminf(fmaxf(u, 0.0f), 1.0f);
                    v = fminf(fmaxf(v, 0.0f), 1.0f);
                    data[(y * w + x) * 3 + 0] = u;
                    data[(y * w + x) * 3 + 1] = v;
                    data[(y * w + x) * 3 + 2] = 0;
                } else if (type == "mip") {
                    vec3 c = mip_colors[0];
                    data[(y * w + x) * 3 + 0] = c.x;
                    data[(y * w + x) * 3 + 1] = c.y;
                    data[(y * w + x) * 3 + 2] = c.z;
                    continue;
                } else {
                    float iw = e_23 * verts[0].coord.w + e_31 * verts[1].coord.w + e_12 * verts[2].coord.w;
                    float u  = (e_23 * verts[0].uv.x + e_31 * verts[1].uv.x + e_12 * verts[2].uv.x) / iw;
                    float v  = (e_23 * verts[0].uv.y + e_31 * verts[1].uv.y + e_12 * verts[2].uv.y) / iw;
                    vec3 tc = interpolate(tex->copies[0], tex->cop_w[0], tex->cop_h[0], make2(u, 1.0f - v));
                    data[(y * w + x) * 3 + 0] = tc.x;
                    data[(y * w + x) * 3 + 1] = tc.y;
                    data[(y * w + x) * 3 + 2] = tc.z;
                }
            }
        }
    }
}