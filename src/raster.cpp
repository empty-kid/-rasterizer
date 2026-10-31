#include <mesh_utils.h>

static float max(float a, float b) {
    return a > b ? a : b;
}

static float min(float a, float b) {
    return a < b ? a : b;
}

static float cross_p(vec2 p, vec2 v1, vec2 v2) {
    return (v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x);
}

vec3 interpolate(const float* tex, int tw, int th, vec2 t) {
    t.x -= floorf(t.x);
    t.y -= floorf(t.y);
    
    // if (t.x > 1) {
    //     t.x = 1;
    // } else if(t.x < -1) {
    //     t.x = 0;
    // }
    // if (t.y > 1) {
    //     t.y = 1;
    // } else if(t.y < -1) {
    //     t.y = 0;
    // }
    t.y = 1.0f - t.y;
    float x = t.x * tw - 0.5f;
    float y = t.y * th - 0.5f;
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    float alpha = x - x0;
    float beta = y - y0;

    x0 = x0 > 0 ? x0 : tw - 1;
    int x1 = x0 + 1 < tw ? x0 + 1 : x0 + 1 - tw;
    y0 = y0 > 0 ? y0 : th - 1;
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

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 c1, vec3 c2, vec3 c3, bool back_sides) {
    float s = cross_p(v3, v1, v2);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(v1.x, max(v2.x, v3.x));
    int y_max = (int)max(v1.y, max(v2.y, v3.y));
    int x_min = (int)min(v1.x, min(v2.x, v3.x));
    int y_min = (int)min(v1.y, min(v2.y, v3.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, v1, v2) / s;
            float e_23 = cross_p(v_center, v2, v3) / s;
            float e_31 = cross_p(v_center, v3, v1) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = z1 * e_23 + z2 * e_31 + z3 * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                float r = c1.x * e_23 + c2.x * e_31 + c3.x * e_12;
                float g = c1.y * e_23 + c2.y * e_31 + c3.y * e_12;
                float b = c1.z * e_23 + c2.z * e_31 + c3.z * e_12;
                data[(y * w + x) * 3 + 0] = r;
                data[(y * w + x) * 3 + 1] = g;
                data[(y * w + x) * 3 + 2] = b;
            }
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, bool back_sides) {
    float s = cross_p(v3, v1, v2);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(v1.x, max(v2.x, v3.x));
    int y_max = (int)max(v1.y, max(v2.y, v3.y));
    int x_min = (int)min(v1.x, min(v2.x, v3.x));
    int y_min = (int)min(v1.y, min(v2.y, v3.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, v1, v2) / s;
            float e_23 = cross_p(v_center, v2, v3) / s;
            float e_31 = cross_p(v_center, v3, v1) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = z1 * e_23 + z2 * e_31 + z3 * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                vec3 n;
                n.x = n1.x * e_23 + n2.x * e_31 + n3.x * e_12;
                n.y = n1.y * e_23 + n2.y * e_31 + n3.y * e_12;
                n.z = n1.z * e_23 + n2.z * e_31 + n3.z * e_12;
                normalize(n);
                data[(y * w + x) * 3 + 0] = (n.x + 1) / 2;
                data[(y * w + x) * 3 + 1] = (n.y + 1) / 2;
                data[(y * w + x) * 3 + 2] = (n.z + 1) / 2;
            }
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, vec2 uv1, vec2 uv2, vec2 uv3, float iw1, float iw2, float iw3, bool back_sides) {
    float s = cross_p(v3, v1, v2);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(v1.x, max(v2.x, v3.x));
    int y_max = (int)max(v1.y, max(v2.y, v3.y));
    int x_min = (int)min(v1.x, min(v2.x, v3.x));
    int y_min = (int)min(v1.y, min(v2.y, v3.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, v1, v2) / s;
            float e_23 = cross_p(v_center, v2, v3) / s;
            float e_31 = cross_p(v_center, v3, v1) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = z1 * e_23 + z2 * e_31 + z3 * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                float iw = e_23 * iw1 + e_31 * iw2 + e_12 *iw3;
                float u  = (e_23 * uv1.x * iw1 + e_31 * uv2.x * iw2 + e_12 * uv3.x * iw3) / iw;
                float v  = (e_23 * uv1.y * iw1 + e_31 * uv2.y * iw2 + e_12 * uv3.y * iw3) / iw;
                u -= floorf(u);
                v -= floorf(v);
                data[(y * w + x) * 3 + 0] = u;
                data[(y * w + x) * 3 + 1] = v;
                data[(y * w + x) * 3 + 2] = 0;
            }
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, vec2 uv1, vec2 uv2, vec2 uv3, float iw1, float iw2, float iw3, const float *img, int tw, int th, bool back_sides) {
    float s = cross_p(v3, v1, v2);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    int x_max = (int)max(v1.x, max(v2.x, v3.x));
    int y_max = (int)max(v1.y, max(v2.y, v3.y));
    int x_min = (int)min(v1.x, min(v2.x, v3.x));
    int y_min = (int)min(v1.y, min(v2.y, v3.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int x = x_min; x <= x_max; x++) {
        for (int y = y_min; y <= y_max; y++) {
            vec2 v_center = make2((float)x + 0.5, (float)y + 0.5);
            float e_12 = cross_p(v_center, v1, v2) / s;
            float e_23 = cross_p(v_center, v2, v3) / s;
            float e_31 = cross_p(v_center, v3, v1) / s;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = z1 * e_23 + z2 * e_31 + z3 * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                float iw = e_23 * iw1 + e_31 * iw2 + e_12 * iw3;
                float u  = (e_23 * uv1.x + e_31 * uv2.x + e_12 * uv3.x) / iw;
                float v  = (e_23 * uv1.y + e_31 * uv2.y  + e_12 * uv3.y) / iw;
                vec3 tc = interpolate(img, tw, th, make2(u, v));
                vec3 n;
                n.x = n1.x * e_23 + n2.x * e_31 + n3.x * e_12;
                n.y = n1.y * e_23 + n2.y * e_31 + n3.y * e_12;
                n.z = n1.z * e_23 + n2.z * e_31 + n3.z * e_12;
                normalize(n);
                float nl = dot(n, l) > 0 ? dot(n, l) : 0;
                data[(y * w + x) * 3 + 0] = tc.x * (k_a.x + k_d.x * nl);
                data[(y * w + x) * 3 + 1] = tc.y * (k_a.y + k_d.y * nl);
                data[(y * w + x) * 3 + 2] = tc.z * (k_a.z + k_d.z * nl);
            }
        }
    }
}
