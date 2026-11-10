#include <texturing.h>

void create_texture(texture &tex, const char* tex_path)
{
    int w{}, h{};
    tex.copies.emplace_back(load_image_f32_rgb(tex_path, &w, &h, 2.2));
    tex.cop_w.emplace_back(w);
    tex.cop_h.emplace_back(h);
    while (w > 1 || h > 1) {
        int nw = (w + 1) / 2;
        int nh = (h + 1) / 2;
        float *copy = (float *)malloc(sizeof(float) * (size_t)nw * nh * 3);
        float *prev = tex.copies.back();
        for (int y = 0; y < nh; y++) {
            for (int x = 0; x < nw; x++) {
                int x0 = x * 2;
                int x1 = x0 + 1;
                int y0 = y * 2;
                int y1 = y0 + 1;
                if (x1 >= w) x1 = w - 1;
                if (y1 >= h) y1 = h - 1;
                for (int c = 0; c < 3; c++) {
                    copy[(y * nw + x) * 3 + c] = (prev[(y0 * w + x0) * 3 + c] + prev[(y0 * w + x1) * 3 + c] + prev[(y1 * w + x0) * 3 + c] + prev[(y1 * w + x1) * 3 + c]) / 4.0f;
                }
            }
        }
        tex.copies.emplace_back(copy);
        tex.cop_w.push_back(nw);
        tex.cop_h.push_back(nh);
        w = nw;
        h = nh;
    }
}