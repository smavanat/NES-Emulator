#ifndef __SHADER_H__
#define __SHADER_H__
#include "include/glad.h"
#include <stdbool.h>

typedef struct {
    int id;
} shader;

#ifdef __cplusplus
extern "C" {
#endif
shader *load_shader(char *vertexPath, char *fragmentPath);

void use(shader *shader);

void set_bool(shader *s, char *name, bool value);
void set_int(shader *s, char *name, int value);
void set_float(shader *s, char *name, float value);

#ifdef __cplusplus
}
#endif
#endif
