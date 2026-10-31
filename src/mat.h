#include <vectors.h>

template <int N>
matn<N> transpose(const matn<N> &a) {
    matn<N> b;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            b.mat[i][j] = a.mat[j][i];
        }
    }
    return b;
}

template <int N>
matn<N> sum(const matn<N> &a, const matn<N> &b) {
    matn<N> c;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            c.mat[i][j] += a.mat[i][j] + b.mat[i][j];
        }
    }
    return c;
}

template <int N>
matn<N> mul(const matn<N> &a, const matn<N> &b) {
    matn<N> c;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                c.mat[i][j] += a.mat[i][k] * b.mat[k][j];
            }
        }
    }
    return c;
}

vec4 mul(const mat4 &a, const vec4 &b) {
    vec4 c;
    for (int i = 0; i < 4; i++) {
        c.M[i] = 0;
        for (int j = 0; j < 4; j++) {
            c.M[i] += a.mat[i][j] * b.M[j];
        }
    }
    return c;
}

vec3 mul(const mat3 &a, const vec3 &b) {
    vec3 c;
    for (int i = 0; i < 3; i++) {
        c.M[i] = 0;
        for (int j = 0; j < 3; j++) {
            c.M[i] += a.mat[i][j] * b.M[j];
        }
    }
    return c;
}

vec3 add(vec3 &a, vec3 &b) {
    vec3 c;
    for (int i = 0; i < 3; i++) {
        c.M[i] = a.M[i] + b.M[i];
    }
    return c;
}

vec3 sub(vec3 &a, vec3 &b) {
    vec3 c;
    for (int i = 0; i < 3; i++) {
        c.M[i] = a.M[i] - b.M[i];
    }
    return c;
}

template <int N>
bool inverse(matn<N> a, matn<N> &inv) {
    for (int i = 0; i < N; i++) {
        inv.mat[i][i] = 1;
    }
    for(int i = 0; i < N; i++) {
        float pivot = a.mat[i][i];
        int pivot_row = i;
        for (int k = i + 1; k < N; k++) {
            if (std::fabs(a.mat[k][i]) > std::fabs(pivot)) {
                pivot = a.mat[k][i];
                pivot_row = k;
            }
        }
        if (std::fabs(pivot) < 1e-9) {
            return false;
        }
        if (pivot_row != i) {
            for (int j = 0; j < N; j++) {
                float tmp = a.mat[i][j];
                a.mat[i][j] = a.mat[pivot_row][j];
                a.mat[pivot_row][j] = tmp;
                tmp = inv.mat[i][j];
                inv.mat[i][j] = inv.mat[pivot_row][j];
                inv.mat[pivot_row][j] = tmp;
            }
        }
        float div = a.mat[i][i];
        for (int j = 0; j < N; j++) {
            a.mat[i][j] /= div;
            inv.mat[i][j] /= div;
        }
        for (int k = 0; k < N; k++) {
            if (k != i) {
                float factor = a.mat[k][i];
                for (int j = 0; j < N; j++) {
                    a.mat[k][j] -= factor * a.mat[i][j];
                    inv.mat[k][j] -= factor * inv.mat[i][j];
                }
            }
        }
    }
    return true;
}