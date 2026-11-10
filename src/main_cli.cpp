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
#include <string.h>
#include <vector>

void paint(std::vector<float> &fb, std::vector<float> &sb, std::vector<vec4> &clip, std::vector<float> &iws, std::vector<vec2> &uvs_iw, std::vector<vec3> &normals, const mat4 &V, const mat4 &P, const vec3 &l, std::vector<object> &objs, int W, int H) {
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

        if (type == "full" or type == "uv") {
            iws.resize(objs[q].m->num_vertices);
            uvs_iw.resize(objs[q].m->num_vertices);
        }
        for (int i = 0; i < objs[q].m->num_vertices; i++) {
            vec4 p = {objs[q].m->verts[i].x, objs[q].m->verts[i].y, objs[q].m->verts[i].z, 1.0f};
            clip[i] = mul(MVP, p);
            vec4 normal4 = mul(mul(V, transpose(inv_M)), make4(objs[q].m->normals[i].x, objs[q].m->normals[i].y, objs[q].m->normals[i].z, 0));
            normals[i].x = normal4.x;
            normals[i].y = normal4.y;
            normals[i].z = normal4.z;
            if (type == "full" or type == "uv") {
                iws[i] = 1.0f / clip[i].w;
                uvs_iw[i].x = objs[q].m->tcs[i].x / clip[i].w;
                uvs_iw[i].y = objs[q].m->tcs[i].y / clip[i].w;
            }
        }

        for (int t = 0; t < objs[q].m->num_triangles; t++) {
            int idx[3] = {objs[q].m->indices[3*t + 0], objs[q].m->indices[3*t + 1], objs[q].m->indices[3*t + 2]};
            std::vector<vec3> verts(3);
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
                verts[j].x = ((p.x / p.w) / 2 + 0.5f) * W;
                verts[j].y = ((p.y / p.w) / 2 + 0.5f) * H;
                verts[j].z = p.z / p.w;
            }
            if (fl) {
                continue;
            }
            if (type == "depth") {
                rasterisation(fb.data(), sb.data(), W, H, verts,
                make3(1,0,0), make3(0,1,0), make3(0,0,1), true);
            } else if (type == "normals") {
                rasterisation(fb.data(), sb.data(), W, H, verts,
                normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1),  true);
            } else if (type == "uv") {
                rasterisation(fb.data(), sb.data(), W, H, verts, 
                normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), uvs_iw[idx[0]], uvs_iw[idx[1]], uvs_iw[idx[2]], iws[idx[0]], iws[idx[1]], iws[idx[2]], true);
            } else {
                rasterisation(fb.data(), sb.data(), W, H, verts,
                normals[idx[0]], normals[idx[1]], normals[idx[2]], l, make3(0.15, 0.15, 0.15), make3(1, 1, 1), uvs_iw[idx[0]], uvs_iw[idx[1]], uvs_iw[idx[2]], iws[idx[0]], iws[idx[1]], iws[idx[2]], objs[q].tex, objs[q].w, objs[q].h, true);
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *out_path = argc > 1 ? argv[1] : "res";
    int count = argc > 2 ? int(strtol(argv[2], NULL, 0)) : 1;
    const char *obj_path = argc > 3 ? argv[3] : "resources/Bunny.obj";
    const char *tex_path = argc > 4 ? argv[4] : "resources/textures/porcelain.jpg";
    std::string type = argc > 5 ? argv[5] : "full";
    std::vector<mesh> meshs;
    meshs.reserve(count);
    int count_meshs{};
    std::vector<float *> texs;
    int count_texs{};
    std::vector<object> objs;
    for (int i = 0; i < count; i++) {
        object obj;
        obj.type = argv[5 + i * 3];
        for (int j = 0; j < i; j++) {
            if (strcmp(argv[3 + i * 3], argv[3 + j * 3]) == 0) {
                obj.m = objs[j].m;
            } 
            if (strcmp(argv[4 + i * 3], argv[4 + j * 3]) == 0) {
                obj.tex = objs[j].tex;
            }
        }
        if (obj.m == NULL) {
            meshs.emplace_back(load_obj(argv[3 + i * 3]));
            obj.m = &(meshs[count_meshs]);
            count_meshs++;

            if (obj.m->num_triangles == 0) {
                printf("Failed to load mesh: %s\n", obj_path);
                for (int r = 0; r < count_meshs; r++) {
                    free_mesh(&meshs[r]);
                }
                for (int r = 0; r < count_texs; r++) {
                    free(texs[r]);
                }
                return 1;
            }

            if (obj.m->normals == NULL) {
                vec3 *acc = (vec3 *)calloc(obj.m->num_vertices, sizeof(vec3));
                for(int t = 0; t < obj.m->num_triangles; t++) {
                    int i0 = obj.m->indices[3*t];
                    int i1 = obj.m->indices[3*t + 1];
                    int i2 = obj.m->indices[3*t + 2];
                    vec3 e1 = sub(obj.m->verts[i1], obj.m->verts[i0]);
                    vec3 e2 = sub(obj.m->verts[i2], obj.m->verts[i0]);
                    vec3 n = cross(e1, e2);
                    acc[i0] = add(acc[i0], n);
                    acc[i1] = add(acc[i1], n);
                    acc[i2] = add(acc[i2], n);
                }
                for (int i = 0; i < obj.m->num_vertices; i++)
                    normalize(acc[i]);
                obj.m->normals = acc;
            }
        }

        if (obj.tex == NULL && obj.type == "full") {
            texs.emplace_back(load_image_f32_rgb(argv[4 + i * 3], &obj.w, &obj.h, 2.2));
            obj.tex = texs[count_texs];
            count_texs++;
        }
        objs.push_back(obj);
    }

    int W = 720;
    int H = 480;
    std::vector<float> fb((size_t)W * H * 3, 0.0f);
    std::vector<float> sb((size_t)W * H, 1.0f);
    std::vector<vec4> clip(meshs[0].num_vertices);
    std::vector<float> iws;
    std::vector<vec2> uvs_iw;
    std::vector<vec3> normals(meshs[0].num_vertices);
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
    
    float max_crds[3] = {meshs[0].verts[0].x, meshs[0].verts[0].y, meshs[0].verts[0].z};
    float min_crds[3] = {meshs[0].verts[0].x, meshs[0].verts[0].y, meshs[0].verts[0].z};
    for (int i = 1; i < meshs[0].num_vertices; i++) {
        for (int j = 0; j < 3; j++) {
            if (meshs[0].verts[i].M[j] > max_crds[j]) {
                max_crds[j] = meshs[0].verts[i].M[j];
            } else if (meshs[0].verts[i].M[j] < min_crds[j]) {
                min_crds[j] = meshs[0].verts[i].M[j];
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
    
    if (count == 1) {
        objs[0].M = mul(make_scale(make3(1.0f / max_size, 1.0f / max_size, 1.0f / max_size)), make_translate(make3(-center.x, -center.y, -center.z)));
    } else {
        for (int i = 0; i < count; i++) {
            float offset = 2.5f * (i - (count - 1) / 2.0f); 
            objs[i].M = mul(make_scale(make3(1.0f / (max_size * 1.75), 1.0f / (max_size * 1.75), 1.0f / (max_size * 1.75))), make_translate(make3(offset, 0.0f, 0.0f)));    
        }
    }

    paint(fb, sb, clip, iws, uvs_iw, normals, V, P, l, objs, W, H);

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

    for (int i = 0; i < count_meshs; i++) {
        free_mesh(&meshs[i]);
    }
    for (int i = 0; i < count_texs; i++) {
        free(texs[i]);
    }
    // printf("Mesh %s\n", obj_path);
    // printf("  vertices:   %d\n", m.num_vertices);
    // printf("  triangles:  %d\n", m.num_triangles);
    // printf("  normals:    %s\n", m.normals ? "yes" : "no");
    // printf("  tex coords: %s\n", m.tcs ? "yes" : "no");
    // free_mesh(&m);
    // type = argc > 1 ? out_path : "full";
    std::string out = out_path;
    out += ".png";
    

    const int res = save_image_f32_png_rgb(fb.data(), out.c_str(), W, H, 3, 2.2f);
    
    if (!res)
    {
        printf("Failed to save image: %s\n", out_path);
        return 1;
    }

    printf("Saved image with swapped R and B channels to %s\n", out_path);
    return 0;
}
