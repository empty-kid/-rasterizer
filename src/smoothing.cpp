#include "smoothing.h"

#include <algorithm>

static constexpr float T_abs = 0.0833;
static constexpr float T_rel = 0.166;
static constexpr float FXAA_QUALITY_P[5] = {1.0f, 1.5f, 2.0f, 4.0f, 12.0f};

float max(float a, float b, float c, float d, float e)
{
    float mx = a;
    mx = b > mx ? b : mx;
    mx = c > mx ? c : mx;
    mx = d > mx ? d : mx;
    mx = e > mx ? e : mx;
    return mx;
}

float min(float a, float b, float c, float d, float e)
{
    float mn = a;
    mn = b < mn ? b : mn;
    mn = c < mn ? c : mn;
    mn = d < mn ? d : mn;
    mn = e < mn ? e : mn;
    return mn;
}

float max(float a, float b)
{
    return a > b ? a : b;
}

float smoothstep(float t)
{
    return t * t * (3 - 2 * t);
}

void fxaa(std::vector<float> &fb, std::vector<float> &tb, int W, int H)
{
    auto get_luma = [&](int px, int py) -> float {
        if (px < 0 || px >= W || py < 0 || py >= H) return 0.0f;
        return dot(make3(fb[((size_t)py * W + px) * 3 + 0], fb[((size_t)py * W + px) * 3 + 1], fb[((size_t)py * W + px) * 3 + 2]), make3(0.299, 0.587, 0.114));
    };
    auto interpolate = [&](float px, float py) -> float {
        if (px < 0 || px > W - 1 || py < 0 || py > H - 1) return 0.0f;
        int x1 = (int)floorf(px);
        int x2 = (int)ceil(px);
        int y1 = (int)floorf(py);
        int y2 = (int)ceil(py);
        float alpha = px - floorf(px);
        float beta = py - floorf(py);
        float lume_00 = get_luma(x1, y1);
        float lume_01 = get_luma(x1, y2);
        float lume_10 = get_luma(x2, y1);
        float lume_11 = get_luma(x2, y2);
        return (lume_00 * (1.0f - alpha) + lume_10 * alpha) * (1.0f - beta) + (lume_01 * (1.0f - alpha) + lume_11 * alpha) * beta;
    };
    auto sample_color = [&](float px, float py, int c) -> float {
        float cx = std::clamp(px, 0.0f, (float)(W - 1));
        float cy = std::clamp(py, 0.0f, (float)(H - 1));
        int x1 = (int)floorf(cx);
        int x2 = (int)ceilf(cx);
        int y1 = (int)floorf(cy);
        int y2 = (int)ceilf(cy);
        float alpha = cx - x1;
        float beta = cy - y1;
        float c00 = fb[((size_t)y1 * W + x1) * 3 + c];
        float c10 = fb[((size_t)y1 * W + x2) * 3 + c];
        float c01 = fb[((size_t)y2 * W + x1) * 3 + c];
        float c11 = fb[((size_t)y2 * W + x2) * 3 + c];
        return (c00 * (1.0f - alpha) + c10 * alpha) * (1.0f - beta) + (c01 * (1.0f - alpha) + c11 * alpha) * beta;
    };
#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 1; y < H - 1; y++) {
        for (int x = 1; x < W - 1; x++) {
            float g_m = get_luma(x, y);
            float g_n = get_luma(x, y + 1); 
            float g_s = get_luma(x, y - 1);
            float g_w = get_luma(x - 1, y);
            float g_e = get_luma(x + 1, y);
            float g_max = max(g_m, g_n, g_s, g_w, g_e);
            float g_min = min(g_m, g_n, g_s, g_w, g_e);
            float r = g_max - g_min;
            if (r < max(T_abs, T_rel * g_max)) {
                continue;
            }
            float g_nw = get_luma(x - 1, y + 1);
            float g_ne = get_luma(x + 1, y + 1);
            float g_sw = get_luma(x - 1, y - 1);
            float g_se = get_luma(x + 1, y - 1);
            float g_hor = std::fabs(g_nw + g_sw - 2.0f * g_w) + 2.0f * std::fabs(g_n + g_s - 2.0f * g_m) + std::fabs(g_ne + g_se - 2.0f * g_e);
            float g_ver = std::fabs(g_nw + g_ne - 2.0f * g_n) + 2.0f * std::fabs(g_w + g_e - 2.0f * g_m) + std::fabs(g_sw + g_se - 2.0f * g_s);

            int dx{}, dy{};
            int blend_dx{}, blend_dy{};

            if (g_hor >= g_ver) {
                dx = 1;
                dy = 0;
                blend_dy = (std::abs(g_n - g_m) >= std::abs(g_s - g_m)) ? +1 : -1;
            } else {
                dx = 0;
                dy = 1;
                blend_dx = (std::abs(g_e - g_m) >= std::abs(g_w - g_m)) ? +1 : -1;
            }

            float gradient = g_hor >= g_ver ? max(std::abs(g_n - g_m), std::abs(g_s - g_m)) : max(std::abs(g_e - g_m), std::abs(g_w - g_m));
            float gradient_threshold = gradient * 0.25f;
            float luma_pair = g_m + (g_hor >= g_ver ? (blend_dy > 0 ? g_n : g_s) : (blend_dx > 0 ? g_e : g_w));
            float d1 = 0.0f;
            float d2 = 0.0f;
            float e1 = 0.0f;
            float e2 = 0.0f;
            bool done1{}, done2{};
            float x1 = (float)x, x2 = (float)x;
            float y1 = (float)y, y2 = (float)y;
            for (int step = 0; step < 5; step++) {
                if (!done1) {
                    x1 += (float)dx * FXAA_QUALITY_P[step];
                    y1 += (float)dy * FXAA_QUALITY_P[step];
                    if (x1 < 0 || x1 >= W || y1 < 0 || y1 >= H) {
                        done1 = true;
                    } else {
                        e1 = interpolate(x1, y1) - 0.5f * luma_pair;
                        d1 = std::abs(x - x1) + std::abs(y - y1);
                        if (std::abs(e1) >= gradient_threshold) done1 = true;
                    }
                }
                if (!done2) {
                    x2 -= (float)dx * FXAA_QUALITY_P[step];
                    y2 -= (float)dy * FXAA_QUALITY_P[step];
                    if (x2 < 0 || x2 >= W || y2 < 0 || y2 >= H) {
                        done2 = true;
                    } else {
                        e2 = interpolate(x2, y2) - 0.5f * luma_pair;
                        d2 = std::abs(x - x2) + std::abs(y - y2);
                        if (std::abs(e2) >= gradient_threshold) done2 = true;
                    }
                }
            }

            float d_min = std::min(d1, d2);
            bool luma_mlt_zero = (g_m - 0.5f * luma_pair) < 0.0f;
            float e_near = (d1 < d2) ? e1 : e2;
            bool good_span = (e_near < 0.0f) != luma_mlt_zero;
            float pixel_offset = d1 + d2 > 0.0f ? 0.5f - d_min / (d1 + d2) : 0.0f;

            float g_avg = (2.0f * (g_e + g_w + g_n + g_s) + g_ne + g_nw + g_se + g_sw) / 12.0f;
            float t_sub = std::clamp(std::fabs(g_avg - g_m) / r, 0.0f, 1.0f);
            float sub = smoothstep(t_sub);
            float offset = std::max(good_span ? pixel_offset : 0.0f, 0.75f * sub * sub);
            float sx = (float)x + (float)blend_dx * (0.5f + offset);
            float sy = (float)y + (float)blend_dy * (0.5f + offset);

            for (int c = 0; c < 3; ++c) {
                tb[((size_t)y * W + x) * 3 + c] = sample_color(sx, sy, c);
            }
        }
    }
}

void fxaa_inter(std::vector<float> &fb, std::vector<float> &tb, std::vector<float> &lb, int W, int H)
{
#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < (size_t)W * H; i++) {
        float l = dot(make3(fb[i * 3 + 0], fb[i * 3 + 1], fb[i * 3 + 2]), make3(0.299f, 0.587f, 0.114f));
        lb[i] = std::clamp(l, 0.0f, 1.0f);
    }
    auto get_luma = [&](int px, int py) -> float {
        return lb[(size_t)py * W + px];
    };
    auto sample_color = [&](float px, float py, int c) -> float {
        float cx = std::clamp(px, 0.0f, (float)(W - 1));
        float cy = std::clamp(py, 0.0f, (float)(H - 1));
        int x1 = (int)floorf(cx);
        int x2 = (int)ceilf(cx);
        int y1 = (int)floorf(cy);
        int y2 = (int)ceilf(cy);
        float alpha = cx - x1;
        float beta = cy - y1;
        float c00 = fb[((size_t)y1 * W + x1) * 3 + c];
        float c10 = fb[((size_t)y1 * W + x2) * 3 + c];
        float c01 = fb[((size_t)y2 * W + x1) * 3 + c];
        float c11 = fb[((size_t)y2 * W + x2) * 3 + c];
        return (c00 * (1.0f - alpha) + c10 * alpha) * (1.0f - beta) + (c01 * (1.0f - alpha) + c11 * alpha) * beta;
    };
    auto interpolate = [&](float px, float py) -> float {
        float cx = std::clamp(px, 0.0f, (float)(W - 1));
        float cy = std::clamp(py, 0.0f, (float)(H - 1));
        int x0 = (int)floorf(cx);
        int y0 = (int)floorf(cy);
        int x1 = std::min(x0 + 1, W - 1);
        int y1 = std::min(y0 + 1, H - 1);
        float ax = cx - x0;
        float ay = cy - y0;
        float a = get_luma(x0, y0);
        float b = get_luma(x1, y0);
        float c = get_luma(x0, y1);
        float d = get_luma(x1, y1);
        return (a * (1.0f - ax) + b * ax) * (1.0f - ay) + (c * (1.0f - ax) + d * ax) * ay;
    };

#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 1; y < H - 1; y++) {
        for (int x = 1; x < W - 1; x++) {
            float g_m = get_luma(x, y);
            float g_n = get_luma(x, y + 1);
            float g_s = get_luma(x, y - 1);
            float g_w = get_luma(x - 1, y);
            float g_e = get_luma(x + 1, y);
            float g_max = max(g_m, g_n, g_s, g_w, g_e);
            float g_min = min(g_m, g_n, g_s, g_w, g_e);
            float r = g_max - g_min;
            if (r < max(T_abs, T_rel * g_max)) {
                continue;
            }
            float g_nw = get_luma(x - 1, y + 1);
            float g_ne = get_luma(x + 1, y + 1);
            float g_sw = get_luma(x - 1, y - 1);
            float g_se = get_luma(x + 1, y - 1);
            float g_hor = std::fabs(g_nw + g_sw - 2.0f * g_w) + 2.0f * std::fabs(g_n + g_s - 2.0f * g_m) + std::fabs(g_ne + g_se - 2.0f * g_e);
            float g_ver = std::fabs(g_nw + g_ne - 2.0f * g_n) + 2.0f * std::fabs(g_w + g_e - 2.0f * g_m) + std::fabs(g_sw + g_se - 2.0f * g_s);

            int dx{}, dy{};
            int blend_dx{}, blend_dy{};

            if (g_hor >= g_ver) {
                dx = 1;
                dy = 0;
                blend_dy = (std::abs(g_n - g_m) >= std::abs(g_s - g_m)) ? +1 : -1;
            } else {
                dx = 0;
                dy = 1;
                blend_dx = (std::abs(g_e - g_m) >= std::abs(g_w - g_m)) ? +1 : -1;
            }

            float gradient = g_hor >= g_ver ? max(std::abs(g_n - g_m), std::abs(g_s - g_m)) : max(std::abs(g_e - g_m), std::abs(g_w - g_m));
            float gradient_threshold = gradient * 0.25f;
            float luma_pair = g_m + (g_hor >= g_ver ? (blend_dy > 0 ? g_n : g_s) : (blend_dx > 0 ? g_e : g_w));
            float d1 = 0.0f;
            float d2 = 0.0f;
            float e1 = 0.0f;
            float e2 = 0.0f;
            bool done1{}, done2{};
            float x1 = (float)x, x2 = (float)x;
            float y1 = (float)y, y2 = (float)y;
            for (int step = 0; step < 5; step++) {
                if (!done1) {
                    x1 += (float)dx * FXAA_QUALITY_P[step];
                    y1 += (float)dy * FXAA_QUALITY_P[step];
                    if (x1 < 0 || x1 >= W || y1 < 0 || y1 >= H) {
                        done1 = true;
                    } else {
                        e1 = interpolate(x1, y1) - 0.5f * luma_pair;
                        d1 = std::abs(x - x1) + std::abs(y - y1);
                        if (std::abs(e1) >= gradient_threshold) done1 = true;
                    }
                }
                if (!done2) {
                    x2 -= (float)dx * FXAA_QUALITY_P[step];
                    y2 -= (float)dy * FXAA_QUALITY_P[step];
                    if (x2 < 0 || x2 >= W || y2 < 0 || y2 >= H) {
                        done2 = true;
                    } else {
                        e2 = interpolate(x2, y2) - 0.5f * luma_pair;
                        d2 = std::abs(x - x2) + std::abs(y - y2);
                        if (std::abs(e2) >= gradient_threshold) done2 = true;
                    }
                }
            }

            float d_min = std::min(d1, d2);
            bool luma_mlt_zero = (g_m - 0.5f * luma_pair) < 0.0f;
            float e_near = (d1 < d2) ? e1 : e2;
            bool good_span = (e_near < 0.0f) != luma_mlt_zero;
            float pixel_offset = d1 + d2 > 0.0f ? 0.5f - d_min / (d1 + d2) : 0.0f;

            float g_avg = (2.0f * (g_e + g_w + g_n + g_s) + g_ne + g_nw + g_se + g_sw) / 12.0f;
            float t_sub = std::clamp(std::fabs(g_avg - g_m) / r, 0.0f, 1.0f);
            float sub = smoothstep(t_sub);
            float offset = std::max(good_span ? pixel_offset : 0.0f, 0.75f * sub * sub);
            float sx = (float)x + (float)blend_dx * (0.5f + offset);
            float sy = (float)y + (float)blend_dy * (0.5f + offset);

            for (int c = 0; c < 3; ++c) {
                tb[((size_t)y * W + x) * 3 + c] = sample_color(sx, sy, c);
            }
        }
    }
}