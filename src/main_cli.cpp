// Пример работы с утилитами шаблона без окна:
// загружает меш, печатает его размеры, загружает текстуру и сохраняет её
// с переставленными местами красным и синим каналами.

#include "image_utils.h"
#include "mesh_utils.h"
#include "mat.h"
#include "raster.h"

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

void paint(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<float> &iws, std::vector<vec2> &uvs_iw, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &MS, int W, int H) {
    int sz = MS.size();
    for (int q = 0; q < sz; q++) {
        normals.resize(MS[q].m->num_vertices);
        mat4 M = MS[q].M;
        std::string type = MS[q].type;
        if ((type == "full" or type == "uv") && MS[q].m->tcs == NULL) {
            type = "normals";
        }
        mat4 MVP  = mul(P, mul(V, M));
        mat4 inv_M;
        inverse(M, inv_M);

        if (type == "full" or type == "uv") {
            iws.resize(MS[q].m->num_vertices);
            uvs_iw.resize(MS[q].m->num_vertices);
        }
        for (int i = 0; i < MS[q].m->num_vertices; i++) {
            vec4 p = {MS[q].m->verts[i].x, MS[q].m->verts[i].y, MS[q].m->verts[i].z, 1.0f};
            clip[i] = mul(MVP, p);
            vec4 normal4 = mul(mul(V, transpose(inv_M)), make4(MS[q].m->normals[i].x, MS[q].m->normals[i].y, MS[q].m->normals[i].z, 0));
            normals[i].x = normal4.x;
            normals[i].y = normal4.y;
            normals[i].z = normal4.z;
            if (type == "full" or type == "uv") {
                iws[i] = 1.0f / clip[i].w;
                uvs_iw[i].x = MS[q].m->tcs[i].x / clip[i].w;
                uvs_iw[i].y = MS[q].m->tcs[i].y / clip[i].w;
            }
        }

        for (int t = 0; t < MS[q].m->num_triangles; t++) {
            int idx[3] = {MS[q].m->indices[3*t + 0], MS[q].m->indices[3*t + 1], MS[q].m->indices[3*t + 2]};
            float s[9]{};
            bool fl{};
            for (int j = 0; j < 3; j++) {
                vec4 &p = clip[idx[j]];
                if (p.w <= 0.0f) {
                    fl = true;
                    break;
                }
                if ((p.x > p.w or p.x < -p.w) or (p.y > p.w or p.y < -p.w) or (p.z > p.w or p.z < -p.w)) {
                    fl = true;
                    break;
                }
                s[j * 3 + 0] = ((p.x / p.w) / 2 + 0.5f) * W;
                s[j * 3 + 1] = ((p.y / p.w) / 2 + 0.5f) * H;
                s[j * 3 + 2] = p.z / p.w;
            }
            if (fl) {
                continue;
            }
            if (type == "depth") {
                rasterisation(fb.data(), sb.data(), W, H,
                make2(s[0], s[1]), make2(s[3], s[4]), make2(s[6], s[7]),
                s[2], s[5], s[8],
                make3(1,0,0), make3(0,1,0), make3(0,0,1), true);
            } else if (type == "normals") {
                rasterisation(fb.data(), sb.data(), W, H,
                make2(s[0], s[1]), make2(s[3], s[4]), make2(s[6], s[7]),
                s[2], s[5], s[8], normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1),  true);
            } else if (type == "uv") {
                rasterisation(fb.data(), sb.data(), W, H,
                make2(s[0], s[1]), make2(s[3], s[4]), make2(s[6], s[7]),
                s[2], s[5], s[8], normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), uvs_iw[idx[0]], uvs_iw[idx[1]], uvs_iw[idx[2]], iws[idx[0]], iws[idx[1]], iws[idx[2]], true);
            } else {
                rasterisation(fb.data(), sb.data(), W, H,
                make2(s[0], s[1]), make2(s[3], s[4]), make2(s[6], s[7]),
                s[2], s[5], s[8], normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), uvs_iw[idx[0]], uvs_iw[idx[1]], uvs_iw[idx[2]], iws[idx[0]], iws[idx[1]], iws[idx[2]], MS[q].tex, MS[q].w, MS[q].h, true);
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *obj_path = argc > 1 ? argv[1] : "resources/Bunny.obj";
    const char *tex_path = argc > 2 ? argv[2] : "resources/textures/porcelain.jpg";
    const char *out_path = argc > 3 ? argv[3] : "res";
    std::string type = argc > 4 ? argv[4] : "full";
    int count = argc > 5 ? int(strtol(argv[5], NULL, 0)) : 1;

    // if (argc > 4)
    // {
    //     printf("Usage: %s [mesh.obj] [texture.png|jpg] [output.png]\n", argv[0]);
    //     return 1;
    // }

    mesh m = load_obj(obj_path);
    mesh m2 = load_obj("resources/Roman Centurion.obj");
    if (m.num_triangles == 0)
    {
        printf("Failed to load mesh: %s\n", obj_path);
        free_mesh(&m);
        return 1;
    }

    if (m.normals == NULL) {
        vec3 *acc = (vec3 *)calloc(m.num_vertices, sizeof(vec3));
        for(int t = 0; t < m.num_triangles; t++) {
            int i0 = m.indices[3*t];
            int i1 = m.indices[3*t + 1];
            int i2 = m.indices[3*t + 2];
            vec3 e1 = sub(m.verts[i1], m.verts[i0]);
            vec3 e2 = sub(m.verts[i2], m.verts[i0]);
            vec3 n = cross(e1, e2);
            acc[i0] = add(acc[i0], n);
            acc[i1] = add(acc[i1], n);
            acc[i2] = add(acc[i2], n);
        }
        for (int i = 0; i < m.num_vertices; i++)
            normalize(acc[i]);
        m.normals = acc;
    }
    
    float max_crds[3] = {m.verts[0].x, m.verts[0].y, m.verts[0].z};
    float min_crds[3] = {m.verts[0].x, m.verts[0].y, m.verts[0].z};
    for (int i = 1; i < m.num_vertices; i++) {
        for (int j = 0; j < 3; j++) {
            if (m.verts[i].M[j] > max_crds[j]) {
                max_crds[j] = m.verts[i].M[j];
            } else if (m.verts[i].M[j] < min_crds[j]) {
                min_crds[j] = m.verts[i].M[j];
            }
        }
    }
    vec3 center;
    float max_size{};
    for (int i = 0; i < 3; i++) {
        center.M[i] = (max_crds[i] + min_crds[i]) / 2;
        if (max_crds[i] - min_crds[i] > max_size) {
            max_size = max_crds[i] - min_crds[i];
        }
    }

    int W = 1920;
    int H = 1080;
    std::vector<float> fb((size_t)W * H * 3, 0.0f);
    std::vector<float> sb((size_t)W * H, 1.0f);
    std::vector<vec4> clip(m.num_vertices);
    std::vector<float> iws;
    std::vector<vec2> uvs_iw;
    std::vector<vec3> normals(m.num_vertices);
    mat4 V = make_look_at(make3(0, 0, 2), make3(0, 0, 0), make3(0, 1, 0));
    mat4 P = make_perspective(45.0f, (float)W / H, 0.1f, 100.0f);
    vec3 l = make3(1, 1, 1);
    normalize(l);
    vec4 l4 = mul(V, make4(l.x, l.y, l.z, 0));
    l.x = l4.x;
    l.y = l4.y;
    l.z = l4.z;
    normalize(l);
    bool depth_mode{};
    std::vector<object> MS(count);
    object N;
    if (count == 1) {
        mat4 M = mul(make_scale(make3(1.0f / max_size, 1.0f / max_size, 1.0f / max_size)), make_translate(make3(-center.x, -center.y, -center.z)));
        float *img{};
        int w = 0, h = 0;
        if (type == "depth") {
            depth_mode = true;
        }
        if (type == "full") {
            img = load_image_f32_rgb(tex_path, &w, &h, 2.2f);
        }
        MS[0] = make_obj(&m, M, img, w, h, type);
    } else {
        MS.resize(count);
        for (int i = 0; i < count; i++) {
            float offset = (i - (count - 1) / 2.0f) * 2.0f; 
            mat4 M = mul(make_scale(make3(1.0f / (max_size * 1.75), 1.0f / (max_size * 1.75), 1.0f / (max_size * 1.75))), make_translate(make3(offset, 0.0f, 0.0f)));
            float *img{};
            int w = 0, h = 0;
            if (i != 0) {
                type = argv[7 + (i - 1) * 2];
            }
            if (type == "full") {
                if (i == 0) {
                    img = load_image_f32_rgb(tex_path, &w, &h, 2.2f);
                } else {
                    img = load_image_f32_rgb(argv[6 + (i - 1) * 2], &w, &h, 2.2f);
                }
            }
            if (type == "depth") {
                depth_mode = true;
            }
            if (i == count - 1) {
                M = mul(make_rotate(-40, 1), M);
                MS[i] = make_obj(&m2, M, img, w, h, type); 
            }
            else {
                MS[i] = make_obj(&m, M, img, w, h, type); 
            }
        }
    }

    paint(fb, sb, clip, iws, uvs_iw, normals, V, P, l, MS, W, H);
    if (depth_mode) {
        float zmin = 1.0f;
        float zmax = -1.0f;
        for (int i = 0; i < W * H; i++) {
            if (sb[i] < 1.0f) {
                zmin = sb[i] < zmin ? sb[i] : zmin;
                zmax = sb[i] > zmax ? sb[i] : zmax;
            }
        }
        std::cout << zmin << " " << zmax << std::endl;
        for (int i = 0; i < W * H; i++) {
            float z;
            if (zmax != zmin) {
                z = (sb[i] - zmin) / (zmax - zmin);
            } else {
                z = 0.0f;
            }
            fb[i * 3 + 0] = 1.0f - z;
            fb[i * 3 + 1] = 1.0f - z;
            fb[i * 3 + 2] = 1.0f - z;   
        }
    }
    for (int i = 0; i < count; i++) {
        if (MS[i].tex != NULL) {
            free(MS[i].tex);
        }
    }
    printf("Mesh %s\n", obj_path);
    printf("  vertices:   %d\n", m.num_vertices);
    printf("  triangles:  %d\n", m.num_triangles);
    printf("  normals:    %s\n", m.normals ? "yes" : "no");
    printf("  tex coords: %s\n", m.tcs ? "yes" : "no");
    free_mesh(&m);
    type = argc > 4 ? argv[4] : "full";
    std::string out = out_path;
    out += "_" + type + ".png";
    

    const int res = save_image_f32_png_rgb(fb.data(), out.c_str(), W, H, 3, 2.2f);
    
    if (!res)
    {
        printf("Failed to save image: %s\n", out_path);
        return 1;
    }

    printf("Saved image with swapped R and B channels to %s\n", out_path);
    return 0;
}
