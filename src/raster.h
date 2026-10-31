void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 c1, vec3 c2, vec3 c3, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, vec2 uv1, vec2 uv2, vec2 uv3, float iw1, float iw2, float iw3, bool back_sides);

void rasterisation(float *data, float *z_buf, int w, int h, vec2 v1, vec2 v2, vec2 v3, float z1, float z2, float z3, vec3 n1, vec3 n2, vec3 n3, vec3 l, vec3 k_a, vec3 k_d, vec2 uv1, vec2 uv2, vec2 uv3, float iw1, float iw2, float iw3, const float *img, int tw, int th, bool back_sides);

vec3 interpolate(const float* tex, int tw, int th, vec2 t);