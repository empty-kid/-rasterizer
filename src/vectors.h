#ifndef VECTORS_H
#define VECTORS_H
#include <cmath>
#include <numbers>
#include <string>

// Минимальный набор векторных типов, нужный утилитам шаблона.
// Остальную математику (операции над векторами, матрицы) вы пишете сами.

typedef struct
{
    union
    {
        struct
        {
            float x;
            float y;
        };
        float M[2];
    };
} vec2;

typedef struct
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        float M[3];
    };
} vec3;

typedef struct
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
        float M[4];
    };
} vec4;

typedef struct
{
    vec4 coord;
    vec2 uv;
    vec3 normals;
} VERT;

typedef struct
{
    vec4 coord;
    vec2 uv;
    vec3 normals;
    vec3 base_normals;
} VERT_L;

template <int N>
struct matn {
    float mat[N][N] = {};
};

using mat4 = matn<4>;
using mat3 = matn<3>;

// typedef struct
// {
//     mat4 M;
//     float *tex = NULL;
//     int w = 0, h = 0;
//     std::string type{};
// } object;

static inline vec3 cross(vec3 v1, vec3 v2) {
    vec3 res;
    res.x = v1.y * v2.z - v2.y * v1.z;
    res.y = v2.x * v1.z - v1.x * v2.z;
    res.z = v1.x * v2.y - v2.x * v1.y;
    return res;
}

static inline float dot(vec2 v1, vec2 v2) {
    float res{};
    for (int i = 0; i < 2; i++) {
        res += v1.M[i] * v2.M[i];
    }
    return res;
}

static inline float dot(vec3 v1, vec3 v2) {
    float res{0};
    for (int i = 0; i < 3; i++) {
        res += v1.M[i] * v2.M[i];
    }
    return res;
}

static inline void normalize(vec3 &v) {
    double w = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x = v.x / w;
    v.y = v.y / w;
    v.z = v.z / w;
}

static inline vec2 make2(float x, float y)
{
    vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

static inline vec3 make3(float x, float y, float z)
{
    vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static inline vec4 make4(float x, float y, float z, float w)
{
    vec4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

// static inline object make_obj(const mat4 &M, float *tex, int w, int h, const std::string &type)
// {
//     object obj;
//     obj.M = M;
//     obj.tex = tex;
//     obj.w = w;
//     obj.h = h;
//     obj.type = type;
//     return obj;
// }

static inline mat4 make_translate(const vec3 &t) {
    mat4 a;
    for (int i = 0; i < 3; i++) {
        a.mat[i][i] = 1;
        a.mat[i][3] = t.M[i];
    }
    a.mat[3][3] = 1;
    return a;
}

static inline mat4 make_rotate(float angle, int axis) {
    mat4 a;
    float s = std::sin(angle * std::numbers::pi / 180);
    float c = std::cos(angle * std::numbers::pi / 180);
    if (axis == 0) {
        a.mat[0][0] = 1;
        a.mat[1][1] = c;
        a.mat[2][2] = c;
        a.mat[1][2] = -s;
        a.mat[2][1] = s;
    } else if (axis == 1) {
        a.mat[1][1] = 1;
        a.mat[0][0] = c;
        a.mat[2][2] = c;
        a.mat[0][2] = s;
        a.mat[2][0] = -s;
    } else {
        a.mat[2][2] = 1;
        a.mat[0][0] = c;
        a.mat[1][1] = c;
        a.mat[0][1] = -s;
        a.mat[1][0] = s;
    }
    a.mat[3][3] = 1;
    return a;
}

static inline mat4 make_scale(const vec3 &s) {
    mat4 a;
    for (int i = 0; i < 3; i++) {
        a.mat[i][i] = s.M[i];
    }
    a.mat[3][3] = 1;
    return a;
}

static inline mat4 make_look_at(const vec3 &e, const vec3 &a, const vec3 &u) {
    vec3 zc;
    zc.x = e.x - a.x;
    zc.y = e.y - a.y;
    zc.z = e.z - a.z;
    normalize(zc);
    vec3 xc = cross(u, zc);
    normalize(xc);
    vec3 yc = cross(zc, xc);
    mat4 V;
    for (int j = 0; j < 3; j++) {
        V.mat[0][j] = xc.M[j]; 
    }
    for (int j = 0; j < 3; j++) {
        V.mat[1][j] = yc.M[j]; 
    }
    for (int j = 0; j < 3; j++) {
        V.mat[2][j] = zc.M[j]; 
    }
    V.mat[0][3] = -dot(xc, e);
    V.mat[1][3] = -dot(yc, e);
    V.mat[2][3] = -dot(zc, e);
    V.mat[3][3] = 1;
    return V;
}

static inline mat4 make_perspective(float angle, float a, float n, float f) {
    mat4 P;
    float c = 1 / std::tan((angle * std::numbers::pi / 180) / 2);
    P.mat[0][0] = c / a;
    P.mat[1][1] = c;
    P.mat[2][2] = (f + n) / (n - f);
    P.mat[3][2] = -1;
    P.mat[2][3] = 2 * f * n / (n - f);
    return P;
}

#endif
