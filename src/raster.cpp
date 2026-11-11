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

template <typename V>
static void find_dl(vec3 &dl_dx, vec3 &dl_dy, const V (&verts)[3], float s) 
{
    float x0 = verts[0].coord.x;
    float y0 = verts[0].coord.y;
    float x1 = verts[1].coord.x;
    float y1 = verts[1].coord.y;
    float x2 = verts[2].coord.x;
    float y2 = verts[2].coord.y;
    dl_dx.M[0] = (y1 - y2) / s;
    dl_dy.M[0] = (x2 - x1) / s;
    dl_dx.M[1] = (y2 - y0) / s;
    dl_dy.M[1] = (x0 - x2) / s;
    dl_dx.M[2] = (y0 - y1) / s;
    dl_dy.M[2] = (x1 - x0) / s;
}

template <typename V>
const void compute_uv_der(float &du_dx, float &du_dy, float &dv_dx, float &dv_dy, const V (&verts)[3], const vec3 &dl_dx, const vec3 &dl_dy, const vec3 &lambda)
{
    float Q = 0.0f;
    float Nu = 0.0f;
    float Nv = 0.0f;
    float dQdx = 0.0f, dNudx = 0.0f, dNvdx = 0.0f;
    float dQdy = 0.0f, dNudy = 0.0f, dNvdy = 0.0f;

    for (int i = 0; i < 3; ++i) {
        float q = verts[i].coord.w;
        float uow = verts[i].uv.x;
        float vow = verts[i].uv.y;
        Q  += lambda.M[i] * q;
        Nu += lambda.M[i] * uow;
        Nv += lambda.M[i] * vow;
        dQdx  += dl_dx.M[i] * q;
        dNudx += dl_dx.M[i] * uow;
        dNvdx += dl_dx.M[i] * vow;
        dQdy  += dl_dy.M[i] * q;
        dNudy += dl_dy.M[i] * uow;
        dNvdy += dl_dy.M[i] * vow;
    }

    float invQ2 = 1.0f / (Q * Q);

    du_dx = (dNudx * Q - Nu * dQdx) * (invQ2);
    du_dy = (dNudy * Q - Nu * dQdy) * (invQ2);
    dv_dx = (dNvdx * Q - Nv * dQdx) * (invQ2);
    dv_dy = (dNvdy * Q - Nv * dQdy) * (invQ2);
}

template <typename V>
vec3 find_coef_uv(float x, float y, float &u, float &v, const V (&verts)[3], float s) 
{
    vec2 v_center = make2(x, y);
    float e_12 = cross_p(v_center, verts[0].coord, verts[1].coord) / s;
    float e_23 = cross_p(v_center, verts[1].coord, verts[2].coord) / s;
    float e_31 = cross_p(v_center, verts[2].coord, verts[0].coord) / s;
    float iw = e_23 * verts[0].coord.w + e_31 * verts[1].coord.w + e_12 * verts[2].coord.w;
    u = (e_23 * verts[0].uv.x + e_31 * verts[1].uv.x + e_12 * verts[2].uv.x) / iw;
    v = (e_23 * verts[0].uv.y + e_31 * verts[1].uv.y + e_12 * verts[2].uv.y) / iw;
    return make3(e_12, e_23, e_31);
}

vec3 interpolate(const float* tex, int tw, int th, vec2 t, bool clamp) 
{
    if (clamp) {
        if (t.x > 1) t.x = 1;
        else if(t.x < 0) t.x = 0;
        if (t.y > 1) t.y = 1;
        else if(t.y < 0) t.y = 0;
    } else {
        t.x -= floorf(t.x);
        t.y -= floorf(t.y);
    }
    

    t.y = 1.0f - t.y;
    float x = t.x * tw - 0.5f;
    float y = t.y * th - 0.5f;
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    float alpha = x - x0;
    float beta = y - y0;

    if (clamp) {
        if (x1 >= tw) x1 = tw - 1;
        if (y1 >= th) y1 = th - 1;
    } else {
        x0 = (x0 % tw + tw) % tw;
        y0 = (y0 % th + th) % th;
        x1 = (x0 + 1) % tw;
        y1 = (y0 + 1) % th;
    }
    
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
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            float u{}, v{};
            vec3 e = find_coef_uv((float)x + 0.5, (float)y + 0.5, u, v, verts, s);
            float e_12 = e.x;
            float e_23 = e.y;
            float e_31 = e.z;
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
                
                    u -= floorf(u);
                    v -= floorf(v);
                    // u = fminf(fmaxf(u, 0.0f), 1.0f);
                    // v = fminf(fmaxf(v, 0.0f), 1.0f);
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
    vec3 dl_dx;
    vec3 dl_dy;
    find_dl(dl_dx, dl_dy, verts, s);
    int x_max = (int)max(verts[0].coord.x, max(verts[1].coord.x, verts[2].coord.x));
    int y_max = (int)max(verts[0].coord.y, max(verts[1].coord.y, verts[2].coord.y));
    int x_min = (int)min(verts[0].coord.x, min(verts[1].coord.x, verts[2].coord.x));
    int y_min = (int)min(verts[0].coord.y, min(verts[1].coord.y, verts[2].coord.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            float u{}, v{};
            vec3 e = find_coef_uv((float)x + 0.5, (float)y + 0.5, u, v, verts, s);
            float e_12 = e.x;
            float e_23 = e.y;
            float e_31 = e.z;
            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = verts[0].coord.z * e_23 + verts[1].coord.z * e_31 + verts[2].coord.z * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
                
                float du_dx = 0.0f;
                float dv_dx = 0.0f;
                float du_dy = 0.0f;
                float dv_dy = 0.0f;
                compute_uv_der(du_dx, du_dy, dv_dx, dv_dy, verts, dl_dx, dl_dy, make3(e_23, e_31, e_12));
                float rho_x = sqrtf(du_dx * du_dx * tex->cop_w[0] * tex->cop_w[0] + dv_dx * dv_dx * tex->cop_h[0] * tex->cop_h[0]);
                float rho_y = sqrtf(du_dy * du_dy * tex->cop_w[0] * tex->cop_w[0] + dv_dy * dv_dy * tex->cop_h[0] * tex->cop_h[0]);

                float rho = fmaxf(rho_x, rho_y);
                float level = log2f(fmaxf(rho, 1.0f));

                if (level < 0.0f) level = 0;
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
                    vec3 tc1 = interpolate(tex->copies[(size_t)floorf(level)], tex->cop_w[(size_t)floorf(level)], tex->cop_h[(size_t)floorf(level)], make2(u, v), false);
                    vec3 tc2 = interpolate(tex->copies[(size_t)floorf(level) + 1], tex->cop_w[(size_t)floorf(level) + 1], tex->cop_h[(size_t)floorf(level) + 1], make2(u, v), false);
                    float k = 1 - (level - floorf(level));
                    tc = make3(tc1.x * k + tc2.x * (1 - k), tc1.y * k + tc2.y * (1 - k), tc1.z * k + tc2.z * (1 - k));
                } else {
                    tc = interpolate(tex->copies[(size_t)level], tex->cop_w[(size_t)level], tex->cop_h[(size_t)level], make2(u, v), false);
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

void rasterisation(float *data, float *z_buf, int w, int h, const VERT (&verts)[3], const texture *tex, const std::string &type) 
{
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
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            float u{}, v{};
            vec3 e = find_coef_uv((float)x + 0.5, (float)y + 0.5, u, v, verts, s);
            float e_12 = e.x;
            float e_23 = e.y;
            float e_31 = e.z;

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
                    u -= floorf(u);
                    v -= floorf(v);
                    // u = fminf(fmaxf(u, 0.0f), 1.0f);
                    // v = fminf(fmaxf(v, 0.0f), 1.0f);
                    data[(y * w + x) * 3 + 0] = u;
                    data[(y * w + x) * 3 + 1] = v;
                    data[(y * w + x) * 3 + 2] = 0;
                } else if (type == "depth") {
                    continue;
                } else {
                    if (type == "mip") {
                        vec3 c = mip_colors[0];
                        data[(y * w + x) * 3 + 0] = c.x;
                        data[(y * w + x) * 3 + 1] = c.y;
                        data[(y * w + x) * 3 + 2] = c.z;
                        continue;
                    }
                    vec3 tc = interpolate(tex->copies[0], tex->cop_w[0], tex->cop_h[0], make2(u, 1.0f - v), false);
                    data[(y * w + x) * 3 + 0] = tc.x;
                    data[(y * w + x) * 3 + 1] = tc.y;
                    data[(y * w + x) * 3 + 2] = tc.z;
                }
            }
        }
    }
}

void rasterisation(float *data, float *z_buf, int w, int h, const VERT_L (&verts)[3], vec3 l, vec3 k_a, vec3 k_d, const texture *tex, const texture &add_tex, const std::string &type, bool back_sides)
{
    float s = cross_p(verts[2].coord, verts[0].coord, verts[1].coord);
    if (fabs(s) < 1e-6f or (back_sides && s < 0)) {
        return;
    }
    vec3 dl_dx;
    vec3 dl_dy;
    find_dl(dl_dx, dl_dy, verts, s);
    int x_max = (int)max(verts[0].coord.x, max(verts[1].coord.x, verts[2].coord.x));
    int y_max = (int)max(verts[0].coord.y, max(verts[1].coord.y, verts[2].coord.y));
    int x_min = (int)min(verts[0].coord.x, min(verts[1].coord.x, verts[2].coord.x));
    int y_min = (int)min(verts[0].coord.y, min(verts[1].coord.y, verts[2].coord.y));
    x_max = x_max < w ? x_max : w - 1;
    y_max = y_max < h ? y_max : h - 1;
    x_min = x_min > 0 ? x_min : 0;
    y_min = y_min > 0 ? y_min : 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            float u{}, v{};
            vec3 e = find_coef_uv((float)x + 0.5, (float)y + 0.5, u, v, verts, s);
            float e_12 = e.x;
            float e_23 = e.y;
            float e_31 = e.z;

            if ((e_12 >= 0 && e_23 >= 0 && e_31 >= 0) || (e_12 <= 0 && e_23 <= 0 && e_31 <= 0)) {
                float z = verts[0].coord.z * e_23 + verts[1].coord.z * e_31 + verts[2].coord.z * e_12;
                if (z > z_buf[y * w + x]) {
                    continue;
                } else {
                    z_buf[y * w + x] = z;
                }
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
                        u -= floorf(u);
                        v -= floorf(v);
                        // u = fminf(fmaxf(u, 0.0f), 1.0f);
                        // v = fminf(fmaxf(v, 0.0f), 1.0f);
                        data[(y * w + x) * 3 + 0] = u;
                        data[(y * w + x) * 3 + 1] = v;
                        data[(y * w + x) * 3 + 2] = 0;
                    } else if (type == "depth") {
                        continue;
                    } else {
                        float du_dx = 0.0f;
                        float dv_dx = 0.0f;
                        float du_dy = 0.0f;
                        float dv_dy = 0.0f;
                        compute_uv_der(du_dx, du_dy, dv_dx, dv_dy, verts, dl_dx, dl_dy, make3(e_23, e_31, e_12));
                        float rho_x = sqrtf(du_dx * du_dx * tex->cop_w[0] * tex->cop_w[0] + dv_dx * dv_dx * tex->cop_h[0] * tex->cop_h[0]);
                        float rho_y = sqrtf(du_dy * du_dy * tex->cop_w[0] * tex->cop_w[0] + dv_dy * dv_dy * tex->cop_h[0] * tex->cop_h[0]);

                        float rho = fmaxf(rho_x, rho_y);
                        float level = log2f(fmaxf(rho, 1.0f));

                        if (level < 0.0f) level = 0.0f;
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
                        vec3 add_tc;
                        if (level != floorf(level)) {
                            vec3 tc1 = interpolate(tex->copies[(size_t)floorf(level)], tex->cop_w[(size_t)floorf(level)], tex->cop_h[(size_t)floorf(level)], make2(u, v), false);
                            vec3 tc2 = interpolate(tex->copies[(size_t)floorf(level) + 1], tex->cop_w[(size_t)floorf(level) + 1], tex->cop_h[(size_t)floorf(level) + 1], make2(u, v), false);
                            float k = 1 - (level - floorf(level));
                            tc = make3(tc1.x * k + tc2.x * (1 - k), tc1.y * k + tc2.y * (1 - k), tc1.z * k + tc2.z * (1 - k));

                            vec3 add_tc1 = interpolate(add_tex.copies[(size_t)floorf(level)], add_tex.cop_w[(size_t)floorf(level)], add_tex.cop_h[(size_t)floorf(level)], make2(u, v), false);
                            vec3 add_tc2 = interpolate(add_tex.copies[(size_t)floorf(level) + 1], add_tex.cop_w[(size_t)floorf(level) + 1], add_tex.cop_h[(size_t)floorf(level) + 1], make2(u, v), false);
                            k = 1 - (level - floorf(level));
                            add_tc = make3(add_tc1.x * k + add_tc2.x * (1 - k), add_tc1.y * k + add_tc2.y * (1 - k), add_tc1.z * k + add_tc2.z * (1 - k));
                        } else {
                            tc = interpolate(tex->copies[(size_t)level], tex->cop_w[(size_t)level], tex->cop_h[(size_t)level], make2(u, v), false);
                            add_tc = interpolate(add_tex.copies[(size_t)level], add_tex.cop_w[(size_t)level], add_tex.cop_h[(size_t)level], make2(u, v), false);
                        }
                        vec3 n;
                        n.x = verts[0].normals.x * e_23 + verts[1].normals.x * e_31 + verts[2].normals.x * e_12;
                        n.y = verts[0].normals.y * e_23 + verts[1].normals.y * e_31 + verts[2].normals.y * e_12;
                        n.z = verts[0].normals.z * e_23 + verts[1].normals.z * e_31 + verts[2].normals.z * e_12;
                        normalize(n);
                        float base_normal_y = verts[0].base_normals.y * e_23 + verts[1].base_normals.y * e_31 + verts[2].base_normals.y * e_12;
                        float nl = dot(n, l) > 0 ? dot(n, l) : 0;
                        data[(y * w + x) * 3 + 0] = tc.x * (k_a.x + k_d.x * nl) * base_normal_y + add_tc.x * (k_a.x + k_d.x * nl) * (1 - base_normal_y);
                        data[(y * w + x) * 3 + 1] = tc.y * (k_a.y + k_d.y * nl) * base_normal_y + add_tc.y * (k_a.y + k_d.y * nl) * (1 - base_normal_y);
                        data[(y * w + x) * 3 + 2] = tc.z * (k_a.z + k_d.z * nl) * base_normal_y + add_tc.z * (k_a.z + k_d.z * nl) * (1 - base_normal_y);
                    }
                }
            }
        }
    }
}