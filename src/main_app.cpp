// Пример вывода массива пикселей в окно в реальном времени.
//
// Каждый кадр программа на CPU заполняет буфер RGBA8 цветом, который плавно
// меняется со временем, и выводит его в окно через glDrawPixels. OpenGL здесь
// используется только для того, чтобы показать готовый массив пикселей.
// Раз в секунду в консоль печатается среднее время кадра.

#include <GLFW/glfw3.h>
#include "image_utils.h"
#include "mesh_utils.h"
#include "mat.h"
#include "raster.h"
#include "texturing.h"
#include <visualization.h>
#include <noise.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <algorithm>

const float SENSIVITY = 0.05f;
float last_x{};
float last_y{};
float yaw{};
float pitch{};
bool first_mouse{true};
bool left_mouse_down{};
bool right_mouse_down{};
bool mouse_look{};
std::string TYPE = "full";
std::string CAM_TYPE = "base";

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_1) {
        TYPE = "full";
    } else if (key == GLFW_KEY_2) {
        TYPE = "uv";
    } else if (key == GLFW_KEY_3) {
        TYPE = "normals";
    } else if (key == GLFW_KEY_4) {
        TYPE = "depth";
    } else if (key == GLFW_KEY_5) {
        TYPE = "mip";
    } else if (key == GLFW_KEY_6) {
        CAM_TYPE = "height";
    } else if (key == GLFW_KEY_7) {
        CAM_TYPE = "base";
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)   left_mouse_down = true;
        if (action == GLFW_RELEASE) left_mouse_down = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)   right_mouse_down = true;
        if (action == GLFW_RELEASE) right_mouse_down = false;
    }
    mouse_look = left_mouse_down || right_mouse_down;
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (first_mouse) {
        last_x = xpos;
        last_y = ypos;
        first_mouse = false;
        return;
    }
    float dx = (float)xpos - last_x;
    float dy = (float)ypos - last_y;
    last_x = xpos;
    last_y = ypos;

    if (!mouse_look) return;

    yaw   -= dx * SENSIVITY;
    pitch -= dy * SENSIVITY;
}

static float clamp01(float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

static uint8_t to_u8(float x)
{
    return (uint8_t)(clamp01(x) * 255.0f + 0.5f);
}

// Цвет радуги по оттенку hue из [0, 1)
static void hue_to_rgb(float hue, float rgb[3])
{
    const float h = hue * 6.0f;
    rgb[0] = clamp01(fabsf(h - 3.0f) - 1.0f);
    rgb[1] = clamp01(2.0f - fabsf(h - 2.0f));
    rgb[2] = clamp01(2.0f - fabsf(h - 4.0f));
}

static void write_rgb(uint8_t *pixels, float r, float g, float b)
{
    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
    r = powf(r, 1.0f / 2.2f);
    g = powf(g, 1.0f / 2.2f);
    b = powf(b, 1.0f / 2.2f);

    pixels[0] = to_u8(r);
    pixels[1] = to_u8(g);
    pixels[2] = to_u8(b);
    pixels[3] = 255; // Alpha
}

static void upscale_nearest(const std::vector<float> &src, uint8_t *dst, int src_w, int src_h, int dst_w, int dst_h)
{
    for (int y = 0; y < dst_h; y++) {
        int src_y = (int)((y + 0.5f) * src_h / dst_h);
        if (src_y >= src_h) src_y = src_h - 1;
        for (int x = 0; x < dst_w; x++) {
            int src_x = (int)((x + 0.5f) * src_w / dst_w);
            if (src_x >= src_w) src_x = src_w - 1;
            const float *c = &src[((size_t)src_y * src_w + src_x) * 3];
            uint8_t *p = &dst[((size_t)y * dst_w + x) * 4];
            write_rgb(p, c[0], c[1], c[2]);
        }
    }
}

static void upscale_bilinear(const std::vector<float> &src, uint8_t *dst, int src_w, int src_h, int dst_w, int dst_h)
{
    for (int y = 0; y < dst_h; y++) {
        float v = (y + 0.5f) / dst_h;
        for (int x = 0; x < dst_w; x++) {
            float u = (x + 0.5f) / dst_w;
            vec3 c = interpolate(src.data(), src_w, src_h, make2(u, 1.0f - v), true);
            uint8_t *p = &dst[((size_t)y * dst_w + x) * 4];
            write_rgb(p, c.x, c.y, c.z);
        }
    }
}

int main(int argc, char **argv)
{
    const int width = argc > 1 ? atoi(argv[1]) : 640;
    const int height = argc > 2 ? atoi(argv[2]) : 480;
    int W = argc > 3 ? atoi(argv[3]) : 640;
    int H = argc > 4 ? atoi(argv[4]) : 480;
    TYPE = argc > 5 ? argv[5] : "full";
    const char *out_path = argc > 6 ? argv[6] : "height_map";
    int count = argc > 7 ? int(strtol(argv[7], NULL, 0)) : 3;
    const char *obj_path = argc > 8 ? argv[8] : "resources/Bunny.obj";
    // const char *tex_path = argc > 9 ? argv[9] : "resources/textures/porcelain.jpg";
    int seed = 42;
    const int hm_weight = 512;
    const int hm_height = 512;
    const float features = 5.0f;
    const int octaves = 6;
    const int grid = 64;
    const int landscape_size = 50;
    const float cell = float(landscape_size) / float(grid);
    const float speed = 1.0f;
    const float landscape_base_y = -2.5f;
    const int period_landscape_tex = 3;


    std::vector<float> fb((size_t)W * H * 3, 0.0f);
    std::vector<float> sb((size_t)W * H, 1.0f);
    vec3 eye = make3(0, 0, 2);
    mat4 V;
    mat4 P = make_perspective(45.0f, (float)W / H, 0.1f, 100.0f);
    vec3 start_light = make3(1, 1, 1);
    normalize(start_light);

    std::vector<object> background;
    make_background(background, 50.0f);

    std::vector<float> height_map((size_t)hm_weight * hm_height);
    Perlin2d noise(seed);

    for (int y = 0; y < hm_height; y++) {
        for (int x = 0; x < hm_weight; x++) {
            float u = ((float)x + 0.5f) / (float)hm_weight * features;
            float v = ((float)y + 0.5f) / (float)hm_height * features;
            float n = noise.noise(u, v, octaves, 0.5f);
            height_map[(size_t)y * hm_weight + x] = 0.5f * (n + 1.0f);
        }
    }
    save_image_f32_png_gray(height_map.data(), out_path, hm_weight, hm_height, 1.0f);
    
    object landscape;
    make_landscape(landscape, grid, cell);

    texture additional_landscape_tex;
    create_texture(additional_landscape_tex, "resources/textures/Grass_11-256x256.png");

    
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Размер буфера пикселей жёстко связан с размером окна, поэтому менять его нельзя
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(width, height, "Software Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);  // ESC
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSwapInterval(0); 

    std::vector<mesh> meshs;
    meshs.reserve(count);
    int count_meshs{};
    std::vector<texture> texs;
    texs.reserve(count);
    int count_texs{};
    std::vector<object> objs;
    for (int i = 0; i < count; i++) {
        object obj;
        obj.type = TYPE;
        for (int j = 0; j < i; j++) {
            if (strcmp(argv[8 + i * 2], argv[8 + j * 2]) == 0) {
                obj.m = objs[j].m;
            } 
            if (strcmp(argv[9 + i * 2], argv[9 + j * 2]) == 0) {
                obj.tex = objs[j].tex;
            }
        }
        if (obj.m == NULL) {
            meshs.emplace_back(load_obj(argv[8 + i * 2]));
            obj.m = &(meshs[count_meshs]);
            count_meshs++;

            if (obj.m->num_triangles == 0) {
                printf("Failed to load mesh: %s\n", obj_path);
                for (int r = 0; r < count_meshs; r++) {
                    free_mesh(&meshs[r]);
                }
                for (int r = 0; r < count_texs; r++) {
                    for (float *p : texs[r].copies) {
                        free(p);
                    }
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

        if (obj.tex == NULL) {
            texture tex;
            create_texture(tex, argv[9 + i * 2]);
            texs.emplace_back(tex);
            obj.tex = &(texs[count_texs]);
            count_texs++;
        }
        objs.push_back(obj);
    }

    int out_w = 0, out_h = 0;
    glfwGetFramebufferSize(window, &out_w, &out_h);
    const size_t n_pixels = (size_t)out_w * out_h;
    uint8_t *pixels = (uint8_t *)malloc(4 * n_pixels);

    
    std::vector<vec4> clip(meshs[0].num_vertices);
    std::vector<vec3> normals(meshs[0].num_vertices);
    std::vector<vec3> base_normals(landscape.m->num_vertices);

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

    double prev_time = glfwGetTime();
    double report_time = prev_time;
    int report_frames = 0;

    while (!glfwWindowShouldClose(window))
    {
        const double now = glfwGetTime();
        const float dt = (float)(now - prev_time); // длительность прошлого кадра, с
        prev_time = now;
        glfwPollEvents();
        mat4 R = mul(make_rotate(yaw, 1), make_rotate(pitch, 0));
        vec4 f4 = mul(R, make4(0, 0, -1, 0));
        vec3 forward = make3(f4.x, f4.y, f4.z);
        normalize(forward);
        vec4 u4 = mul(R, make4(0, 1, 0, 0));
        vec3 up = make3(u4.x, u4.y, u4.z);
        normalize(up);
        vec3 right = cross(forward, up);
        normalize(right);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            eye.x += forward.x * speed * dt;
            eye.y += forward.y * speed * dt;
            eye.z += forward.z * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            eye.x -= forward.x * speed * dt;
            eye.y -= forward.y * speed * dt;
            eye.z -= forward.z * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            eye.x += right.x * speed * dt;
            eye.y += right.y * speed * dt;
            eye.z += right.z * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            eye.x -= right.x * speed * dt;
            eye.y -= right.y * speed * dt;
            eye.z -= right.z * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS and CAM_TYPE != "height") {
            eye.x += up.x * speed * dt;
            eye.y += up.y * speed * dt;
            eye.z += up.z * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS and CAM_TYPE != "height") {
            eye.x -= up.x * speed * dt;
            eye.y -= up.y * speed * dt;
            eye.z -= up.z * speed * dt;
        }
        if (CAM_TYPE == "height") {
            float u = eye.x / landscape_size + 0.5f;
            float v = eye.z / landscape_size + 0.5f;
            float height = sample_height(height_map, hm_weight, hm_height, u, v);
            eye.y = landscape_base_y + height * 3 + 0.5f;
        }
        vec3 target = make3(eye.x + forward.x, eye.y + forward.y, eye.z + forward.z);
        for (object &b : background) {
            b.M = make_translate(eye);
            b.type = TYPE;
        }
        landscape.M = make_translate(make3(roundf(eye.x / cell) * cell, 0, roundf(eye.z / cell) * cell));
        landscape.type = TYPE;
        V = make_look_at(eye, target, up);
        vec4 l4 = mul(V, make4(start_light.x, start_light.y, start_light.z, 0));
        vec3 l = make3(l4.x, l4.y, l4.z);
        normalize(l);
        for (object &obj : objs) {
            obj.type = TYPE;
        }
        std::fill(fb.begin(), fb.end(), 0.0f);
        std::fill(sb.begin(), sb.end(), 1.0f);

        paint(fb, sb, clip, normals, V, P, l, objs, W, H);
        paint_landscape(fb, sb, clip, normals, base_normals, V, P, l, landscape, additional_landscape_tex, W, H, height_map, hm_weight, hm_height, landscape_size, landscape_base_y, period_landscape_tex);
        paint_background(fb, sb, clip, normals, V, P, l, background, W, H);

        // mat4 VP = mul(P, V);
        // mat4 invVP;
        // inverse(VP, invVP);
        // float nx = 2.0f * ((float)x + 0.5f) / W - 1.0f;
        // float ny = 2.0f * ((float)y + 0.5f) / H - 1.0f;

        // vec4 far = mul(invVP, make4(nx, ny, 1.0f, 1.0f));
        // vec3 world = make3(far.x / far.w, far.y / far.w, far.z / far.w);

        // vec3 dir = make3(world.x - eye.x,
        //                 world.y - eye.y,
        //                 world.z - eye.z);
        // normalize(dir);
        // float ax = fabsf(dir.x);
        // float ay = fabsf(dir.y);
        // float az = fabsf(dir.z);

        // int face;
        // if (ax >= ay && ax >= az) {
        //     face = dir.x < 0 ? 3 : 2;
        // } else if (ay >= ax && ay >= az) {
        //     face = dir.y < 0 ? 5 : 4;
        // } else {
        //     face = dir.z < 0 ? 1 : 0;
        // }

        if (TYPE == "depth") {
            float zmin = 1.0f;
            float zmax = -1.0f;
            for (int i = 0; i < W * H; i++) {
                if (sb[i] < 1.0f) {
                    zmin = sb[i] < zmin ? sb[i] : zmin;
                    zmax = sb[i] > zmax ? sb[i] : zmax;
                }
            }
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

        if (out_w % W == 0 && out_h % H == 0) {
            int kx = out_w / W;
            int ky = out_h / H;

            for (int src_y = 0; src_y < H; src_y++) {
                for (int src_x = 0; src_x < W; src_x++) {
                    float *c = &fb[((size_t)src_y * W + src_x) * 3];
                    for (int y = 0; y < ky; y++) {
                        int dst_y = src_y * ky + y;
                        for (int x = 0; x < kx; x++) {
                            int dst_x = src_x * kx + x;
                            uint8_t *p = &pixels[((size_t)dst_y * out_w + dst_x) * 4];
                            write_rgb(p, c[0], c[1], c[2]);
                        }
                    }
                }
            }
        }
        // } else if (out_w != W or out_h != H) {
        //     upscale_bilinear(fb, pixels, W, H, out_w, out_h);
        // }
        if (out_w != W or out_h != H) {
            upscale_nearest(fb, pixels, W, H, out_w, out_h);
        }
        

        glDrawPixels(out_w, out_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glfwSwapBuffers(window);

        report_frames++;
        const double elapsed = glfwGetTime() - report_time;
        if (elapsed >= 1.0)
        {
            printf("frame time: %.3f ms (%.1f FPS)\n",
                   1000.0 * elapsed / report_frames, report_frames / elapsed);
            fflush(stdout);
            report_time += elapsed;
            report_frames = 0;
        }
    }

    free(pixels);
    for (auto& mesh_item : meshs) free_mesh(&mesh_item);
    for (auto &t : texs) {
        for (float *p : t.copies)
        free(p);
    }
    for (auto& b : background) {
        free_mesh(b.m);
        delete b.m;
        for (float *p : b.tex->copies) {
            free(p);
        }
        delete b.tex;
    }
    free_mesh(landscape.m);
    delete landscape.m;
    for (float *p : landscape.tex->copies) {
        free(p);
    }
    delete landscape.tex;
    for (float *p : additional_landscape_tex.copies) {
        free(p);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
