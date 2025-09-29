#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

int read_to_end(char const *path, char **buf, bool addNull) {
    FILE *fp;
    size_t fsz;
    long offEnd;
    int rc;

    //Open the file
    fp = fopen(path, "rb");
    if(NULL == fp) {
        printf("Unable to open file at %s\n", path);
        return -1;
    }

    //Seek to the end of the file
    rc = fseek(fp, 0L, SEEK_END);
    if(0 != rc) {
        printf("Unable to read to end of file at %s\n", path);
        return -1;
    }

    //Byte offset to the end of the file size
    if(0 > (offEnd = ftell(fp))) {
        printf("Byte offset error at %s\n", path);
        return -1;
    }
    fsz = (size_t)offEnd;

    //Allocate a buffer to hold the whole file
    *buf = malloc(fsz + (int)addNull);
    if(NULL == *buf) {
        printf("Failed to create buffer\n");
        return -1;
    }

    //Rewind file pointer to the start of the file:
    rewind(fp);

    //Place the file into a buffer
    if(fsz != fread(*buf, 1, fsz, fp)) {
        printf("File size and buffer size do not match\n");
        free(buf);
        return -1;
    }

    //Close the file
    if(EOF == fclose(fp)) {
        printf("Unable to close the file at %s", path);
        free(*buf);
        return -1;
    }

    //Add null terminator
    if(addNull) {
        (*buf)[fsz] = '\0';
    }

    return fsz;
}

shader *load_shader(char *vertexPath, char *fragmentPath) {
    char *vertexBuf;
    char *fragmentBuf;
    int result;

    result = read_to_end(vertexPath, &vertexBuf, true);
    if(-1 == result) {
        printf("ERROR::VERTEX_SHADER::FILE_NOT_SUCCESSFULLY_READ\n");
        return NULL;
    }

    result = read_to_end(fragmentPath, &fragmentBuf, true);
    if(-1 == result) {
        printf("ERROR::FRAGMENT_SHADER::FILE_NOT_SUCCESSFULLY_READ\n");
        return NULL;
    }

    unsigned int vertex, fragment;
    char infoLog[512];

    //Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char *const *)&vertexBuf, NULL);
    glCompileShader(vertex);

    //print compile errors if any:
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &result);
    if(!result) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n");
        for(int i = 0; i < 512; i++){
            printf("%c", infoLog[i]);
        }
        printf("\n");
    }

    //Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char *const *)&fragmentBuf, NULL);
    glCompileShader(fragment);

    //print compile errors if any:
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &result);
    if(!result) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n");
        for(int i = 0; i < 512; i++){
            printf("%c", infoLog[i]);
        }
        printf("\n");
    }

    int id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    //print errors if any:
    glGetProgramiv(id, GL_LINK_STATUS, &result);
    if(!result) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        printf("ERROR::SHADER::LINKAGE_FAILED\n");
        for(int i = 0; i < 512; i++){
            printf("%c", infoLog[i]);
        }
        printf("\n");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    shader* s = malloc(sizeof(shader));
    s->id = id;
    return s;
}

void use(shader *shader) {
    glUseProgram(shader->id);
}

void set_bool(shader *s, char *name, bool value) {
    glUniform1i(glGetUniformLocation(s->id, name), (int)value);
}
void set_int(shader *s, char *name, int value) {
    glUniform1i(glGetUniformLocation(s->id, name), value);
}
void set_float(shader *s, char *name, float value) {
    glUniform1i(glGetUniformLocation(s->id, name), value);
}
