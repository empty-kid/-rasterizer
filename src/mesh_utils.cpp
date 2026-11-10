#include "mesh_utils.h"
#include "image_utils.h"
#include "texturing.h"
#include "mat.h"

#include <algorithm>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "external/tinyobj_loader_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN64
#define atoll(S) _atoi64(S)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

void free_mesh(mesh *m)
{
    free(m->verts);
    free(m->normals);
    free(m->tcs);
    free(m->indices);
}

static char* mmap_file(size_t* len, const char* filename) {
#ifdef _WIN64
  HANDLE file =
      CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (file == INVALID_HANDLE_VALUE) { /* E.g. Model may not have materials. */
    return NULL;
  }

  HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
  assert(fileMapping != INVALID_HANDLE_VALUE);

  LPVOID fileMapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
  char* fileMapViewChar = (char*)fileMapView;
  assert(fileMapView != NULL);

  DWORD file_size = GetFileSize(file, NULL);
  (*len) = (size_t)file_size;

  return fileMapViewChar;
#else

  struct stat sb;
  char* p;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return NULL;
  }

  if (fstat(fd, &sb) == -1) {
    perror("fstat");
    return NULL;
  }

  if (!S_ISREG(sb.st_mode)) {
    fprintf(stderr, "%s is not a file\n", filename);
    return NULL;
  }

  p = (char*)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

  if (p == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  if (close(fd) == -1) {
    perror("close");
    return NULL;
  }

  (*len) = sb.st_size;

  return p;

#endif
}

static void get_file_data(void* ctx, const char* filename, const int is_mtl,
                          const char* obj_filename, char** data, size_t* len) 
                          {
  // NOTE: If you allocate the buffer with malloc(),
  // You can define your own memory management struct and pass it through `ctx`
  // to store the pointer and free memories at clean up stage(when you quit an
  // app)
  // This example uses mmap(), so no free() required.
  (void)ctx;

  if (!filename) {
    fprintf(stderr, "null filename\n");
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  size_t data_len = 0;

  *data = mmap_file(&data_len, filename);
  (*len) = data_len;
}

typedef struct
{
    int v_idx, vt_idx, vn_idx;
    int id; // -1 - empty
} vertex_slot;

static uint32_t hash_vertex(int v_idx, int vt_idx, int vn_idx)
{
    uint32_t h = (uint32_t)v_idx * 73856093u;
    h ^= (uint32_t)vt_idx * 19349663u;
    h ^= (uint32_t)vn_idx * 83492791u;
    return h;
}

static int valid_index(int idx, unsigned int count)
{
    return idx >= 0 && (unsigned int)idx < count;
}

mesh load_obj(const char *filename)
{
    mesh m;
    m.num_triangles = 0;
    m.num_vertices = 0;
    m.verts = NULL;
    m.normals = NULL;
    m.tcs = NULL;
    m.indices = NULL;

    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes = NULL;
    size_t num_shapes;
    tinyobj_material_t *materials = NULL;
    size_t num_materials;

    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret =
        tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                          &num_materials, filename, get_file_data, NULL, flags);
    if (ret != TINYOBJ_SUCCESS)
        return m;

    const int num_corners = attrib.num_faces;

    int has_normals = attrib.num_normals > 0;
    int has_tcs = attrib.num_texcoords > 0;
    int valid = num_corners % 3 == 0;
    for (int i = 0; i < num_corners && valid; i++)
    {
        valid = valid_index(attrib.faces[i].v_idx, attrib.num_vertices);
        has_normals = has_normals && valid_index(attrib.faces[i].vn_idx, attrib.num_normals);
        has_tcs = has_tcs && valid_index(attrib.faces[i].vt_idx, attrib.num_texcoords);
    }

    if (valid)
    {
        m.verts = (vec3 *)malloc(num_corners * sizeof(vec3));
        m.normals = has_normals ? (vec3 *)malloc(num_corners * sizeof(vec3)) : NULL;
        m.tcs = has_tcs ? (vec2 *)malloc(num_corners * sizeof(vec2)) : NULL;
        m.indices = (int *)malloc(num_corners * sizeof(int));
        m.num_triangles = num_corners/3;

        uint32_t table_size = 1;
        while (table_size < 2 * (uint32_t)num_corners)
            table_size *= 2;
        vertex_slot *table = (vertex_slot*)malloc(table_size * sizeof(vertex_slot));
        for (uint32_t i = 0; i < table_size; i++)
            table[i].id = -1;

        for (int i = 0; i < num_corners; i++)
        {
            const int v_idx = attrib.faces[i].v_idx;
            const int vt_idx = has_tcs ? attrib.faces[i].vt_idx : -1;
            const int vn_idx = has_normals ? attrib.faces[i].vn_idx : -1;

            uint32_t slot = hash_vertex(v_idx, vt_idx, vn_idx) & (table_size - 1);
            while (table[slot].id >= 0 &&
                   !(table[slot].v_idx == v_idx && table[slot].vt_idx == vt_idx && table[slot].vn_idx == vn_idx))
                slot = (slot + 1) & (table_size - 1);

            if (table[slot].id < 0)
            {
                const int id = m.num_vertices++;
                table[slot].v_idx = v_idx;
                table[slot].vt_idx = vt_idx;
                table[slot].vn_idx = vn_idx;
                table[slot].id = id;

                m.verts[id] = make3(attrib.vertices[3 * v_idx + 0], attrib.vertices[3 * v_idx + 1], attrib.vertices[3 * v_idx + 2]);
                if (has_normals)
                    m.normals[id] = make3(attrib.normals[3 * vn_idx + 0], attrib.normals[3 * vn_idx + 1], attrib.normals[3 * vn_idx + 2]);
                if (has_tcs)
                    m.tcs[id] = make2(attrib.texcoords[2 * vt_idx + 0], attrib.texcoords[2 * vt_idx + 1]);
            }

            m.indices[i] = table[slot].id;
        }

        free(table);

        m.verts = (vec3 *)realloc(m.verts, m.num_vertices * sizeof(vec3));
        if (m.normals)
            m.normals = (vec3 *)realloc(m.normals, m.num_vertices * sizeof(vec3));
        if (m.tcs)
            m.tcs = (vec2 *)realloc(m.tcs, m.num_vertices * sizeof(vec2));
    }
    else
    {
        fprintf(stderr, "load_obj: %s has invalid face indices\n", filename);
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    return m;
}

void make_background(std::vector<object> &background, float S)
{
    vec3 corner[8];
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                int v = i * 4 + j * 2 + k;
                corner[v] = make3(-S + 2.0f * S * i, -S + 2.0f * S * j, -S + 2.0f * S * k);
            }
        }
    }
    static const int face_idx[6][4] = {
        {1,5,7,3},
        {0,2,6,4},
        {4,6,7,5},
        {0,1,3,2},
        {2,3,7,6},
        {0,4,5,1},
    };
    static const float face_uv[6][4][2] = {
        { {1,0}, {0,0}, {0,1}, {1,1} },
        { {0,0}, {0,1}, {1,1}, {1,0} },
        { {0,0}, {0,1}, {1,1}, {1,0} },
        { {1,0}, {0,0}, {0,1}, {1,1} },
        { {0,0}, {0,1}, {1,1}, {1,0} },
        { {0,1}, {1,1}, {1,0}, {0,0} },
    };
    static const int face_nrm[6][3] = {
        {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}
    };
    static const char *names[6] = {"ft", "bk", "rt", "lf", "up", "dn" };

    for (int f = 0; f < 6; f++) {
        mesh *m = new mesh{};
        m->num_vertices  = 4;
        m->num_triangles = 2;
        m->verts = (vec3 *)malloc(sizeof(vec3) * 4);
        m->normals = (vec3 *)malloc(sizeof(vec3) * 4);
        m->tcs = (vec2 *)malloc(sizeof(vec2) * 4);
        m->indices = (int  *)malloc(sizeof(int)  * 6);

        for (int i = 0; i < 4; i++) {
            m->verts[i] = corner[face_idx[f][i]];
            m->normals[i] = make3((float)face_nrm[f][0], (float)face_nrm[f][1], (float)face_nrm[f][2]);
            m->tcs[i] = make2(face_uv[f][i][0], face_uv[f][i][1]);
        }

        m->indices[0] = 0; m->indices[1] = 1; m->indices[2] = 2;
        m->indices[3] = 0; m->indices[4] = 2; m->indices[5] = 3;

        texture *t = new texture();
        std::string path = std::string("resources/textures/skybox/hills_") + names[f];
        path += (strcmp(names[f], "up") == 0) ? ".jpg" : ".png";

        int w = 0, h = 0;
        float *tex = load_image_f32_rgb(path.c_str(), &w, &h, 2.2f);
        t->copies.push_back(tex);
        t->cop_w.push_back(w);
        t->cop_h.push_back(h);
        background.emplace_back(make_obj(m, make_translate(make3(0, 0, 0)), t, "full"));
    }
}

mesh* make_landscape_mesh(int grid, float cell)
{
    mesh *m = new mesh{};
    m->num_vertices = (grid + 1) * (grid + 1);
    m->num_triangles = grid * grid * 2;
    m->verts = (vec3 *)malloc(sizeof(vec3) * m->num_vertices);
    m->normals = (vec3 *)malloc(sizeof(vec3) * m->num_vertices);
    m->tcs = (vec2 *)malloc(sizeof(vec2) * m->num_vertices);
    m->indices = (int *)malloc(sizeof(int)  * m->num_triangles * 3);
    size_t t{};
    for (int x = 0; x <= grid; x++) {
        for (int z = 0; z <= grid; z++) {
            m->verts[z * (grid + 1) + x] = make3((x - grid * 0.5f) * cell, 0.0f, (z - grid * 0.5f) * cell);
            if (x != grid && z != grid) {
                int i00 = z * (grid + 1) + x;
                int i10 = i00 + 1;
                int i01 = (z + 1) * (grid + 1) + x;
                int i11 = i01 + 1;

                m->indices[t++] = i00; m->indices[t++] = i01; m->indices[t++] = i11;
                m->indices[t++] = i00; m->indices[t++] = i11; m->indices[t++] = i10;
            }
        }
    }

    vec3 *acc = (vec3 *)calloc(m->num_vertices, sizeof(vec3));
    for (int i = 0; i < m->num_triangles; i++) {
        int i0 = m->indices[3 * i + 0];
        int i1 = m->indices[3 * i + 1];
        int i2 = m->indices[3 * i + 2];

        vec3 e1 = sub(m->verts[i1], m->verts[i0]);
        vec3 e2 = sub(m->verts[i2], m->verts[i0]);
        vec3 n  = cross(e1, e2);

        acc[i0] = add(acc[i0], n);
        acc[i1] = add(acc[i1], n);
        acc[i2] = add(acc[i2], n);
    }
    for (int i = 0; i < m->num_vertices; i++) {
        normalize(acc[i]);
        m->normals[i] = acc[i];
    }
    free(acc);

    return m;
}

void make_landscape(object &landscape, int grid, float cell) {
    landscape.m = make_landscape_mesh(grid, cell);
    landscape.M = mat4();
    landscape.type = "full";
    landscape.tex = new texture();
    const char *tex_path = "resources/textures/Grass_09-256x256.png";
    create_texture(*landscape.tex, tex_path);
}