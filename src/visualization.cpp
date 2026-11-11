#include <visualization.h>

template <typename V>
int clip_near(const V (&in)[3], V (&out)[4])
{
    int n = 0;
    for (int i = 0; i < 3; i++) {
        vec4 a = in[i].coord, b = in[(i + 1) % 3].coord;
        float da = a.z + a.w;
        float db = b.z + b.w;
        if (da >= 0)
        out[n++] = in[i];
        if ((da >= 0) != (db >= 0))
        out[n++] = lerp_vertex(in[i], in[(i + 1) % 3], da / (da - db));
    }
    return n;
}

void paint(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &objs, int W, int H)
{
    int sz = objs.size();
    for (int q = 0; q < sz; q++) {
        clip.resize(objs[q].m->num_vertices);
        normals.resize(objs[q].m->num_vertices);
        mat4 M = objs[q].M;
        std::string type = objs[q].type;
        if ((type == "full" or type == "uv") && objs[q].m->tcs == NULL) {
            type = "normals";
        }
        mat4 MVP  = mul(P, mul(V, M));
        mat4 inv_M;
        inverse(M, inv_M);

        for (int i = 0; i < objs[q].m->num_vertices; i++) {
            vec4 p = {objs[q].m->verts[i].x, objs[q].m->verts[i].y, objs[q].m->verts[i].z, 1.0f};
            clip[i] = mul(MVP, p);
            vec4 normal4 = mul(mul(V, transpose(inv_M)), make4(objs[q].m->normals[i].x, objs[q].m->normals[i].y, objs[q].m->normals[i].z, 0));
            normals[i].x = normal4.x;
            normals[i].y = normal4.y;
            normals[i].z = normal4.z;
        }

        for (int t = 0; t < objs[q].m->num_triangles; t++) {
            int idx[3] = {objs[q].m->indices[3*t + 0], objs[q].m->indices[3*t + 1], objs[q].m->indices[3*t + 2]};
            VERT verts_before[3];
            bool behind_screen{};
            for (int j = 0; j < 3; j++) {
                verts_before[j].coord = clip[idx[j]];
                if (type == "full" or type == "uv") {
                    verts_before[j].uv = objs[q].m->tcs[idx[j]];
                } else {
                    verts_before[j].uv = make2(verts_before[j].coord.x, verts_before[j].coord.y);
                }
                if (verts_before[j].coord.w <= 0.0f) {
                    behind_screen = true;
                    break;
                }
                verts_before[j].normals = normals[idx[j]];
            }
            if (behind_screen) {
                continue;
            }
            VERT verts[4];
            int n;
            n = clip_near(verts_before, verts);
            for (int j = 0; j < n; j++) {
                verts[j].coord.x = ((verts[j].coord.x / verts[j].coord.w) / 2.0f + 0.5f) * W;
                verts[j].coord.y = ((verts[j].coord.y / verts[j].coord.w) / 2.0f + 0.5f) * H;
                verts[j].coord.z = verts[j].coord.z / verts[j].coord.w;
                verts[j].uv.x = verts[j].uv.x / verts[j].coord.w;
                verts[j].uv.y = verts[j].uv.y / verts[j].coord.w;
                verts[j].coord.w = 1 / verts[j].coord.w;
            }
            if (n == 0) {
                continue;
            } else if (n == 3) {
                VERT proc[3] = {verts[0], verts[1], verts[2]};
                if (type != "full" && type != "mip") {
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), type, true);
                } else {
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), objs[q].tex, type, true);
                }
            } else {
                VERT proc[3] = {verts[0], verts[1], verts[2]};
                if (type != "full" && type != "mip") {
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), type, true);
                    proc[1] = verts[2];
                    proc[2] = verts[3];
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), type, true);
                } else {
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), objs[q].tex, type, true);
                    proc[1] = verts[2];
                    proc[2] = verts[3];
                    rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), objs[q].tex, type, true);
                }
            } 
        }
    }
}

void paint_background(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &objs, int W, int H)
{
    int sz = objs.size();
    for (int q = 0; q < sz; q++) {
        clip.resize(objs[q].m->num_vertices);
        normals.resize(objs[q].m->num_vertices);
        mat4 M = objs[q].M;
        std::string type = objs[q].type;
        mat4 MVP  = mul(P, mul(V, M));
        mat4 inv_M;
        inverse(M, inv_M);

        for (int i = 0; i < objs[q].m->num_vertices; i++) {
            vec4 p = {objs[q].m->verts[i].x, objs[q].m->verts[i].y, objs[q].m->verts[i].z, 1.0f};
            clip[i] = mul(MVP, p);
            vec4 normal4 = mul(mul(V, transpose(inv_M)), make4(objs[q].m->normals[i].x, objs[q].m->normals[i].y, objs[q].m->normals[i].z, 0));
            normals[i].x = normal4.x;
            normals[i].y = normal4.y;
            normals[i].z = normal4.z;
        }

        for (int t = 0; t < objs[q].m->num_triangles; t++) {
            int idx[3] = {objs[q].m->indices[3*t + 0], objs[q].m->indices[3*t + 1], objs[q].m->indices[3*t + 2]};
            VERT verts_before[3];
            for (int j = 0; j < 3; j++) {
                verts_before[j].coord = clip[idx[j]];
                if (type == "full" or type == "uv") {
                    verts_before[j].uv = objs[q].m->tcs[idx[j]];
                    verts_before[j].uv.y = verts_before[j].uv.y;
                } else {
                    verts_before[j].uv = make2(verts_before[j].coord.x, verts_before[j].coord.y);
                }
                verts_before[j].normals = normals[idx[j]];
            }
            VERT verts[4];
            int n;
            n = clip_near(verts_before, verts);
            for (int j = 0; j < n; j++) {
                verts[j].coord.x = ((verts[j].coord.x / verts[j].coord.w) / 2.0f + 0.5f) * W;
                verts[j].coord.y = ((verts[j].coord.y / verts[j].coord.w) / 2.0f + 0.5f) * H;
                verts[j].coord.z = verts[j].coord.z / verts[j].coord.w;
                verts[j].uv.x = verts[j].uv.x / verts[j].coord.w;
                verts[j].uv.y = verts[j].uv.y / verts[j].coord.w;
                verts[j].coord.w = 1 / verts[j].coord.w;
            }
            if (n == 0) {
                continue;
            } else if (n == 3) {
                VERT proc[3] = {verts[0], verts[1], verts[2]};
                rasterisation(fb.data(), sb.data(), W, H, proc, objs[q].tex, type);
            } else {
                VERT proc[3] = {verts[0], verts[1], verts[2]};
                rasterisation(fb.data(), sb.data(), W, H, proc, objs[q].tex, type);
                proc[1] = verts[2];
                proc[2] = verts[3];
                rasterisation(fb.data(), sb.data(), W, H, proc, objs[q].tex, type);
            }
        }
    }
}

float sample_height(const std::vector<float> &height_map, int w, int h, float u, float v)
{
    // if (u < 0.0f) u = 0.0f;
    // else if (u > 1.0f) u = 1.0f;
    // if (v < 0.0f) v = 0.0f;
    // else if (v > 1.0f) v = 1.0f;
    u -= floorf(u);
    v -= floorf(v);

    float x = u * (float)(w - 1);
    float y = v * (float)(h - 1);
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1 < w ? x0 + 1 : w - 1;
    int y1 = y0 + 1 < h ? y0 + 1 : h - 1;
    float ax = x - (float)x0;
    float ay = y - (float)y0;

    float h00 = height_map[(size_t)y0 * w + x0];
    float h10 = height_map[(size_t)y0 * w + x1];
    float h01 = height_map[(size_t)y1 * w + x0];
    float h11 = height_map[(size_t)y1 * w + x1];

    return (h00 * (1 - ax) + h10 * ax) * (1 - ay) + (h01 * (1 - ax) + h11 * ax) * ay;
}

static vec3 calculate_normal(const std::vector<float> &hm, int w, int h, float u, float v, float d, int landscape_size)
{
    float lh = sample_height(hm, w, h, u - d, v);
    float rh = sample_height(hm, w, h, u + d, v);
    float uh = sample_height(hm, w, h, u, v + d);
    float dh = sample_height(hm, w, h, u, v - d);
    vec3 n = make3(-3.0f * (rh - lh) / (d * (float)landscape_size * 2.0f), 1.0f, -3.0f * (uh - dh) / (d * (float)landscape_size * 2.0f));
    normalize(n);
    return n;
}

void paint_landscape(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<vec3> &normals, std::vector<vec3> &base_normals, const mat4 &V, const mat4 &P, const vec3 &l, object &landscape, const texture &additional_landscape_tex, int W, int H, const std::vector<float> &height_map, int hm_weight, int hm_height, int landscape_size, float landscape_base_y, int period)
{
    clip.resize(landscape.m->num_vertices);
    normals.resize(landscape.m->num_vertices);
    mat4 M = landscape.M;
    std::string type = landscape.type;
    mat4 inv_M;
    inverse(M, inv_M);
    
    for (int i = 0; i < landscape.m->num_vertices; i++) {
        vec4 p = {landscape.m->verts[i].x, landscape.m->verts[i].y, landscape.m->verts[i].z, 1.0f};
        vec4 world_p = mul(M, p);
        float ux = (world_p.x / landscape_size + 0.5f);
        float vz = (world_p.z / landscape_size + 0.5f);
        float h = sample_height(height_map, hm_weight, hm_height, ux, vz);
        world_p.y = landscape_base_y + h * 3;
        clip[i] = mul(P, mul(V, world_p));
        float d = 1.0f / (float)landscape_size;
        vec3 normal = calculate_normal(height_map, hm_weight, hm_height, ux, vz, d, landscape_size);
        vec4 normal4 = mul(V, make4(normal.x, normal.y, normal.z, 0.0f));
        normals[i] = make3(normal4.x, normal4.y, normal4.z);
        base_normals[i] = normal;
        landscape.m->tcs[i] = make2(ux * (float)period, vz * (float)period);
    }

    for (int t = 0; t < landscape.m->num_triangles; t++) {
        int idx[3] = {landscape.m->indices[3*t + 0], landscape.m->indices[3*t + 1], landscape.m->indices[3*t + 2]};
        VERT_L verts_before[3];
        for (int j = 0; j < 3; j++) {
            verts_before[j].coord = clip[idx[j]];
            verts_before[j].uv = landscape.m->tcs[idx[j]];
            verts_before[j].normals = normals[idx[j]];
            verts_before[j].base_normals = base_normals[idx[j]];
        }
        VERT_L verts[4];
        int n;
        n = clip_near(verts_before, verts);
        for (int j = 0; j < n; j++) {
            verts[j].coord.x = ((verts[j].coord.x / verts[j].coord.w) / 2.0f + 0.5f) * W;
            verts[j].coord.y = ((verts[j].coord.y / verts[j].coord.w) / 2.0f + 0.5f) * H;
            verts[j].coord.z = verts[j].coord.z / verts[j].coord.w;
            verts[j].uv.x = verts[j].uv.x / verts[j].coord.w;
            verts[j].uv.y = verts[j].uv.y / verts[j].coord.w;
            verts[j].coord.w = 1 / verts[j].coord.w;
        }
        if (n == 0) {
            continue;
        } else if (n == 3) {
            VERT_L proc[3] = {verts[0], verts[1], verts[2]};
            rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), landscape.tex, additional_landscape_tex, type, true);
        } else {
            VERT_L proc[3] = {verts[0], verts[1], verts[2]};
            rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), landscape.tex, additional_landscape_tex, type, true);
            proc[1] = verts[2];
            proc[2] = verts[3];
            rasterisation(fb.data(), sb.data(), W, H, proc, l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), landscape.tex, additional_landscape_tex, type, true);
        }
    }
}

void paint_background(std::vector<float> &fb, std::vector<float> &sb, const vec3 &eye, const mat4 &V, const mat4 &P, std::vector<object> &objs, int W, int H)
{
    mat4 VP = mul(P, V);
    mat4 inv_VP;
    inverse(VP, inv_VP);
    for (int j = 0; j < H; j++) {
        for (int i = 0; i < W; i++) {
            size_t idx = (size_t)j * W + i;
            if (sb[idx] < 1.0f) {
                continue;
            }
            float x = 2.0f * ((float)i + 0.5f) / (float)W - 1.0f;
            float y = 2.0f * ((float)j + 0.5f) / (float)H - 1.0f;
            vec4 p = mul(inv_VP, make4(x, y, 1.0f, 1.0f));
            vec3 world = make3(p.x / p.w, p.y / p.w, p.z / p.w);
            vec3 dir = sub(world, eye);
            normalize(dir);
            float ax = fabsf(dir.x);
            float ay = fabsf(dir.y);
            float az = fabsf(dir.z);
            int face;
            float u, v;
            if (az >= ax && az >= ay) {
                if (dir.z > 0.0f) {
                    face = 0;
                    u = 0.5f * (1.0f - dir.x / dir.z);
                    v = 0.5f * (1.0f + dir.y / dir.z);
                } else {
                    face = 1;
                    u = 0.5f * (1.0f - dir.x / dir.z);
                    v = 0.5f * (1.0f - dir.y / dir.z);
                }
            } else if (ax >= ay) {
                if (dir.x > 0.0f) {
                    face = 2;
                    u = 0.5f * (1.0f + dir.z / dir.x);
                    v = 0.5f * (1.0f + dir.y / dir.x);
                } else {
                    face = 3;
                    u = 0.5f * (1.0f + dir.z / dir.x);
                    v = 0.5f * (1.0f - dir.y / dir.x);
                }
            } else {
                if (dir.y > 0.0f) {
                    face = 4;
                    u = 0.5f * (1.0f - dir.x / dir.y);
                    v = 0.5f * (1.0f - dir.z / dir.y);
                } else {
                    face = 5;
                    u = 0.5f * (1.0f - dir.x / dir.y);
                    v = 0.5f * (1.0f + dir.z / dir.y);
                }
            }
            const texture *tex = objs[face].tex;
            vec3 c = interpolate(tex->copies[0], tex->cop_w[0], tex->cop_h[0], make2(u, 1.0f - v), false);
            fb[idx * 3 + 0] = c.x;
            fb[idx * 3 + 1] = c.y;
            fb[idx * 3 + 2] = c.z;
        }
    }
}