#ifndef TEXTURING_H
#define TEXTURING_H

#include "image_utils.h"
#include "mesh_utils.h"
#include "mat.h"
#include "raster.h"
#include <visualization.h>
#include <noise.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void create_texture(texture &tex, const char* tex_path);

#endif