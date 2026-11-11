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
                for (int c = 0; c < 3; c++) {
                    float sum = 0.0f;
                    int count = 0;
                    for (int dy = 0; dy < 2; ++dy) {
                        for (int dx = 0; dx < 2; ++dx) {
                            int sx = 2 * x + dx;
                            int sy = 2 * y + dy;
                            if (sx < w && sy < h) {
                                sum += prev[(sy * w + sx) * 3 + c];
                                ++count;
                            }
                        }
                    }
                    copy[(y * nw + x) * 3 + c] = sum / count;
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