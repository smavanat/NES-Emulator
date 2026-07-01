#include "renderer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../externals/glad.h"

//Calculates the projection matrix
void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            dest[i][j] = 0;
        }
    }

    float rl = 1.0 / (right  - left);
    float tb = 1.0 / (top    - bottom);
    float mfn =-1.0 / (farZ - nearZ);

    dest[0][0] = 2.0 * rl;
    dest[1][1] = 2.0 * tb;
    dest[2][2] = 2.0 * mfn;
    dest[3][0] =-(right  + left) * rl;
    dest[3][1] =-(top    + bottom) * tb;
    dest[3][2] = (farZ + nearZ) * mfn;
    dest[3][3] = 1.0;
}

//Compiles a shader from a source file given the desired shader type
unsigned int create_shader(const char **src, int shader_type) {
    unsigned int shader;
    shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, src, NULL);
    glCompileShader(shader);

    int result;
    char infolog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        glGetShaderInfoLog(shader, 512, NULL, infolog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n");
        for(int i = 0; i < 512; i++){
            if(infolog[i] == '\0') break;
            printf("%c", infolog[i]);
        }
        printf("\n");
    }

    return shader;
}

//Shaders for this program are simple enough that we can just encode them as strings
//to avoid annoying file loading/reading every startup
const char *vertex_shader = "#version 330 core\n"
                            "layout (location = 0) in vec2 aPos;\n"
                            "layout (location = 1) in vec4 aColor;\n"
                            "layout (location = 2) in vec2 aTexCoord;\n"
                            "uniform mat4 uProjection;\n"
                            "out vec4 ourColor;\n"
                            "out vec2 TexCoord;\n"
                            "void main() {\n"
                            "    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);\n"
                            "    ourColor = aColor;"
                            "    TexCoord = aTexCoord;\n"
                            "}\n";
const char *fragment_shader = "#version 330 core\n"
                              "out vec4 FragColor;\n"
                              "in vec2 TexCoord;\n"
                              "in vec4 ourColor;\n"
                              "uniform sampler2D screenTexture;\n"
                              "void main() {\n"
                              "    FragColor = texture(screenTexture, TexCoord) * ourColor;\n"
                              "}\n";

//Initialises the pixel renderer
Renderer render_init(size_t width, size_t height) {
    Renderer r = {0};
    r.screen_height = height;
    r.screen_width = width;

    //Getting the shader for this renderer
    r.shader = glCreateProgram();
    unsigned int vert = create_shader(&vertex_shader, GL_VERTEX_SHADER);
    unsigned int frag = create_shader(&fragment_shader, GL_FRAGMENT_SHADER);
    glAttachShader(r.shader, vert);
    glAttachShader(r.shader, frag);
    glLinkProgram(r.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    glGenVertexArrays(1, &r.vao);
    glBindVertexArray(r.vao);

    //Getting the vbo
    glGenBuffers(1, &r.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICIES * sizeof(Render_Vertex), NULL, GL_DYNAMIC_DRAW);

    //Getting the ebo
    glGenBuffers(1, &r.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * MAX_INDECIES, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offsetof(Render_Vertex, pos)); //Vertex Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offsetof(Render_Vertex, colour)); //Vertex Colour
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offsetof(Render_Vertex, uv)); //UV
    glEnableVertexAttribArray(2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Setting the projection matrix
    ortho(0.0f, r.screen_width, r.screen_height, 0.0f, -1.0f, 1.0f, r.projection);

    r.num_text_atlas = 0;
    r.tex_atlas_capacity = 2;
    r.rb = malloc(r.tex_atlas_capacity * sizeof(AtlasRenderBatch));

    //Make the special debug atlas as the first one
    TextureAtlas *ta = malloc(sizeof(TextureAtlas));
    *ta = atlas_init_blank(1, 1, 4);
    uint8_t white[4] = {255, 255, 255, 255};
    atlas_pack(ta, white, 1, 1, 4);

    add_texture_atlas(&r, ta);

    return r;
}

//Frees a pixel renderer
void render_free(Renderer *r) {
    glDeleteBuffers(1, &r->vbo);
    glDeleteVertexArrays(1, &r->vao);
    glDeleteProgram(r->shader);
    for(int i = 0; i < r->num_text_atlas; i++) {
        free(GET_ATLAS_BATCH(r, i).vertex_data);
        free(GET_ATLAS_BATCH(r, i).index_data);
        atlas_free(GET_ATLAS_BATCH(r, i).a);
    }
    free(r->rb[0].a);
    free(r->rb);
}

//Sets up the variables for renderering to the pbo from the Renderer
void render_begin(Renderer *r) {
    for(int i = 0; i < r->num_text_atlas; i++) {
        GET_ATLAS_BATCH(r, i).index_count = 0;
        GET_ATLAS_BATCH(r, i).vertex_count = 0;
    }
}

//Ends rendering to the current pixel frame
void render_end(Renderer *r) {
    //Shifting the positions according to the projection matrix
    glUseProgram(r->shader);
    int proj_loc = glGetUniformLocation(r->shader, "uProjection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r->projection);

    //Bind all of the arrays and buffers we will reuse over time
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);

    for(int i = 0; i < r->num_text_atlas; i++) {
        //TODO: Check if not redefining the buffer sizes using glBufferData
        //and then passing in arrays with a larger size will cause a crash,
        //and if so, how to get around it. Irrelevant for now
        if(GET_ATLAS_BATCH(r, i).index_count > 0) {
            glBufferSubData(GL_ARRAY_BUFFER, 0, GET_ATLAS_BATCH(r, i).vertex_count * sizeof(Render_Vertex), GET_ATLAS_BATCH(r, i).vertex_data); //Copies the data from renderer's triangle data into the vbo
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, GET_ATLAS_BATCH(r, i).index_count * sizeof(uint32_t), GET_ATLAS_BATCH(r, i).index_data); //Copies the quad data into the vbo

            //Bind the atlas texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, GET_ATLAS_BATCH(r, i).a->texture);
            glUniform1i(glGetUniformLocation(r->shader, "screenTexture"), 0);

            glDrawElements(GL_TRIANGLES, GET_ATLAS_BATCH(r, i).index_count, GL_UNSIGNED_INT, 0); //Make the draw call
        }
    }
}

void render_next_atlas_batch(Renderer *r, uint32_t atlas) {
    if(GET_ATLAS_BATCH(r, atlas).index_count + INDECIES_PER_QUAD >= GET_ATLAS_BATCH(r, atlas).index_size) {
        size_t new_cap = GET_ATLAS_BATCH(r, atlas).index_size * 2;
        uint32_t *temp = calloc(new_cap, sizeof(uint32_t));
        memcpy(temp, GET_ATLAS_BATCH(r, atlas).index_data, sizeof(uint32_t) * GET_ATLAS_BATCH(r, atlas).index_size);
        free(GET_ATLAS_BATCH(r, atlas).index_data);
        GET_ATLAS_BATCH(r, atlas).index_data = temp;
        GET_ATLAS_BATCH(r, atlas).index_size = new_cap;
    }
    if(GET_ATLAS_BATCH(r, atlas).vertex_count + VERTICIES_PER_QUAD >= GET_ATLAS_BATCH(r, atlas).vertex_size) {
        size_t new_cap = GET_ATLAS_BATCH(r, atlas).vertex_size * 2;
        Render_Vertex *temp = calloc(new_cap, sizeof(Render_Vertex));
        memcpy(temp, GET_ATLAS_BATCH(r, atlas).vertex_data, sizeof(Render_Vertex) * GET_ATLAS_BATCH(r, atlas).vertex_size);
        free(GET_ATLAS_BATCH(r, atlas).vertex_data);
        GET_ATLAS_BATCH(r, atlas).vertex_data = temp;
        GET_ATLAS_BATCH(r, atlas).vertex_size = new_cap;
    }
}

uint32_t add_texture_atlas(Renderer *r, TextureAtlas *ta) {
    printf("Number of texture atlases: %zu\n", r->num_text_atlas);
    if(r->num_text_atlas >= r->tex_atlas_capacity) {
        size_t newCap = r->tex_atlas_capacity * 2;
        AtlasRenderBatch *temp = malloc(newCap * sizeof(AtlasRenderBatch));
        memcpy(temp, r->rb, r->num_text_atlas *sizeof(AtlasRenderBatch));
        free(r->rb);
        r->rb = temp;
        r->tex_atlas_capacity = newCap;
    }

    GET_ATLAS_BATCH(r, r->num_text_atlas).index_count = 0;
    GET_ATLAS_BATCH(r, r->num_text_atlas).vertex_count = 0;
    GET_ATLAS_BATCH(r, r->num_text_atlas).index_size = MAX_INDECIES;
    GET_ATLAS_BATCH(r, r->num_text_atlas).vertex_size = MAX_VERTICIES;
    GET_ATLAS_BATCH(r, r->num_text_atlas).index_data = malloc(sizeof(uint32_t) * MAX_INDECIES);
    GET_ATLAS_BATCH(r, r->num_text_atlas).vertex_data = malloc(sizeof(Render_Vertex) * MAX_VERTICIES);
    GET_ATLAS_BATCH(r, r->num_text_atlas).a = ta;

    r->num_text_atlas++;

    return r->num_text_atlas -1;
}

//Draws a texture quad
void render_draw_atlas_quad(Renderer *r, NES_Quad screen_quad, NES_Quad tex_sub_rect, NES_Vector4 colour, uint32_t atlas) {
    //If we have overreached our current rendering limit or we cannot store any more textures, end the current draw call and start a new one
    if(GET_ATLAS_BATCH(r, atlas).vertex_count + VERTICIES_PER_QUAD >= GET_ATLAS_BATCH(r, atlas).vertex_size || GET_ATLAS_BATCH(r, atlas).index_count + INDECIES_PER_QUAD >= GET_ATLAS_BATCH(r, atlas).index_size) {
        render_next_atlas_batch(r, atlas);
    }

    //Update the vertex count and vertex data stored in the renderer
    uint32_t base_index = GET_ATLAS_BATCH(r, atlas).vertex_count;

    NES_Vector2 coords[4] = {
        {screen_quad.x, screen_quad.y},
        {screen_quad.x, screen_quad.y + screen_quad.h},
        {screen_quad.x + screen_quad.w, screen_quad.y + screen_quad.h},
        {screen_quad.x + screen_quad.w , screen_quad.y}
    };

    float width = GET_ATLAS_BATCH(r, atlas).a->width;
    float height = GET_ATLAS_BATCH(r, atlas).a->height;

    NES_Vector2 uv[4] = {
        {tex_sub_rect.x / width, tex_sub_rect.y / height},
        {tex_sub_rect.x / width, (tex_sub_rect.y + tex_sub_rect.h) / height},
        {(tex_sub_rect.x + tex_sub_rect.w) / width, (tex_sub_rect.y + tex_sub_rect.h) / height},
        {(tex_sub_rect.x + tex_sub_rect.w) / width , tex_sub_rect.y / height}
    };

    for(int i = 0; i < VERTICIES_PER_QUAD; i++) {
        GET_ATLAS_BATCH(r, atlas).vertex_data[GET_ATLAS_BATCH(r, atlas).vertex_count++] = (Render_Vertex){coords[i], colour, uv[i]};
    }

    //Need to also add ebo data so we can remove overlapping vertices
    //First triangle
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index;
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index + 1;
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index + 3;

    //Second triangle
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index + 1;
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index + 2;
    GET_ATLAS_BATCH(r, atlas).index_data[GET_ATLAS_BATCH(r, atlas).index_count++] = base_index + 3;
}

void render_draw_texture(Renderer *r, uint32_t texture, NES_Quad dimensions, NES_Quad uv_dimensions, NES_Vector4 colour) {
    glUseProgram(r->shader);

    // Upload projection
    int proj_loc = glGetUniformLocation(r->shader, "uProjection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)r->projection);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(r->shader, "screenTexture"), 0);
    glUniform1i(glGetUniformLocation(r->shader, "useTexture"), 1);

    // Build a single quad directly into a temporary buffer
    Render_Vertex verts[4] = {
        {{dimensions.x,                 dimensions.y               }, colour, {0, 0}},
        {{dimensions.x,                 dimensions.y + dimensions.h}, colour, {0, 1}},
        {{dimensions.x + dimensions.w,  dimensions.y + dimensions.h}, colour, {1, 1}},
        {{dimensions.x + dimensions.w,  dimensions.y               }, colour, {1, 0}},
    };
    uint32_t indices[] = { 0, 1, 3, 1, 2, 3 };

    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void render_draw_pixel_buffer(Renderer *r, PixelBuffer *pb) {
    render_draw_texture(r, pb->pixel_tex, (NES_Quad){0,0,r->screen_width, r->screen_height}, (NES_Quad){0,0,1,1}, (NES_Vector4) {1,1,1,1});
}

//Creates a pixel buffer to hold the pixels representing
//a texture of size width * height
//Pixel size should be either 3 or 4 (rgb/rgba)
PixelBuffer pixelbuffer_init(size_t width, size_t height, uint8_t pixel_size) {
    if(pixel_size != 3 && pixel_size != 4) {
        fprintf(stderr, "Invalid pixel size\n");
        return (PixelBuffer){0};
    }

    PixelBuffer pb = {0};
    pb.width = width;
    pb.height = height;

    //Setting up the texture for the pixel simulations:
    glGenTextures(1, &pb.pixel_tex); //Only use one texture for the pixels that we just write to. Could switch to two and swap them out (like framebuffers)
    glBindTexture(GL_TEXTURE_2D, pb.pixel_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pb.width, pb.height, 0, (pixel_size == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, NULL); //Setting it to use rgba colours
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    pb.buf_sz = width * height * pixel_size;

    //Setting up the pbo for the pixel simulations
    glGenBuffers(1, &pb.pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb.pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, pb.buf_sz, NULL, GL_STREAM_DRAW);
    pb.pixel_buf = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(pb.pixel_buf) {
        memset(pb.pixel_buf, 0x00, pb.buf_sz); //Setting all of the pixels to be colourless initially
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return pb;
}
//Frees the data used by a pixel buffer
void pixelbuffer_free(PixelBuffer *pb) {
    glDeleteBuffers(1, &pb->pbo);
    glDeleteTextures(1, &pb->pixel_tex);
}

//Draws the entire frame directly on the screen by copying its entire contents into the renderer's pixel buffer
void pixelbuffer_updload_data(PixelBuffer *pb, uint8_t *data) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb->pbo);

    void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, data, pb->buf_sz);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, pb->pixel_tex);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pb->width, pb->height, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

//Initialises a texture atlas
//Optionally packs a single white pixel at the start of the texture atlas to render a solid quad
TextureAtlas atlas_init(uint32_t width, uint32_t height, uint32_t texture, uint8_t pixel_size) {
    TextureAtlas a = {0};
    if(pixel_size != 3 && pixel_size != 4) return a;
    a.width = width;
    a.height = height;
    a.pixel_size = pixel_size;
    a.texture = texture;

    return a;
}

TextureAtlas atlas_init_blank(uint32_t width, uint32_t height, uint8_t pixel_size) {
    TextureAtlas a = {0};
    if(pixel_size != 3 && pixel_size != 4) return a;
    a.width = width;
    a.height = height;
    a.pixel_size = pixel_size;

    glGenTextures(1, &a.texture);
    glBindTexture(GL_TEXTURE_2D, a.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, (a.pixel_size == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return a;
}

void atlas_free(TextureAtlas *a) {
    glDeleteTextures(1, &a->texture);
}

//Returns the UV rect where the texture was placed
//pixel_size must be either 3 or 4. If it does not match the pixel size of the atlas,
//an empty quad will returned as the pixel formats are different
NES_Quad atlas_pack(TextureAtlas *a, uint8_t* pixels, size_t w, size_t h, uint8_t pixel_size) {
    //If the atlas' pixel format has not been set, set it to the one passed in
    if(a->pixel_size == 0) a->pixel_size = pixel_size;
    else if(a->pixel_size != pixel_size) return (NES_Quad){0}; //Otherwise there is a mismatch and return an empty quad

    //Move to next row if this texture doesn't fit
    if(a->cursor_x + w > a->width) {
        a->cursor_y += a->row_height;
        a->cursor_x = 0;
        a->row_height = 0;
    }

    //Upload the subregion
    glBindTexture(GL_TEXTURE_2D, a->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, a->cursor_x, a->cursor_y, w, h, (a->pixel_size == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

    //Compute normalised UVs
    NES_Quad uv = {
        (float)a->cursor_x / a->width,
        (float)a->cursor_y / a->height,
        (float) w / a->width,
        (float) h / a->height
    };

    a->cursor_x += w;
    if(h > a->row_height) a->row_height = h;

    return uv;
}

void draw_triangle_strip(Renderer *r, NES_Vector2 strip[4], NES_Vector4 colour) {
    if(GET_ATLAS_BATCH(r, 0).vertex_count + VERTICIES_PER_QUAD >= GET_ATLAS_BATCH(r, 0).vertex_size || GET_ATLAS_BATCH(r, 0).index_count + INDECIES_PER_QUAD >= GET_ATLAS_BATCH(r, 0).index_size) {
        render_next_atlas_batch(r, 0);
    }

    //Update the vertex count and vertex data stored in the renderer
    uint32_t base_index = GET_ATLAS_BATCH(r, 0).vertex_count;

    for(int i = 0; i < VERTICIES_PER_QUAD; i++) {
        GET_ATLAS_BATCH(r, 0).vertex_data[GET_ATLAS_BATCH(r, 0).vertex_count++] = (Render_Vertex){strip[i], colour};
    }

    //Need to also add ebo data so we can remove overlapping vertices
    //First triangle
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index;
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index + 1;
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index + 2;

    //Second triangle
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index + 1;
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index + 2;
    GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = base_index + 3;
}


void render_draw_line(Renderer *r, NES_Vector2 start_pos, NES_Vector2 end_pos, float thickness, NES_Vector4 colour) {
    NES_Vector2 delta = {end_pos.x - start_pos.x, end_pos.y - start_pos.y};
    float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

    if(length > 0 && thickness > 0) {
        float scale = thickness/(2*length);

        NES_Vector2 radius = {-scale*delta.y, scale*delta.x};
        NES_Vector2 strip[4] = {
            {start_pos.x - radius.x, start_pos.y - radius.y},
            {start_pos.x + radius.x, start_pos.y + radius.y},
            {end_pos.x - radius.x, end_pos.y - radius.y},
            {end_pos.x + radius.x, end_pos.y + radius.y},
        };

        draw_triangle_strip(r, strip, colour);
    }
}

void render_draw_quad(Renderer *r, NES_Quad quad, NES_Vector4 colour) {
    NES_Vector2 strip[4] = {
        {quad.x, quad.y},
        {quad.x, quad.y+quad.h},
        {quad.x+quad.w, quad.y},
        {quad.x+quad.w, quad.y+quad.h},
    };

    draw_triangle_strip(r, strip, colour);
}

void render_draw_quad_bordered(Renderer *r, NES_Quad quad, NES_Vector4 q_col, NES_Vector4 b_col, float thick) {
    render_draw_quad(r, quad, q_col);
    render_draw_unfilled_quad(r, quad, thick, b_col);
}

void render_draw_unfilled_quad(Renderer *r, NES_Quad quad, float thickness, NES_Vector4 colour) {
    NES_Vector2 tl = {quad.x,          quad.y};
    NES_Vector2 tr = {quad.x + quad.w, quad.y};
    NES_Vector2 bl = {quad.x,          quad.y + quad.h};
    NES_Vector2 br = {quad.x + quad.w, quad.y + quad.h};

    render_draw_line(r, tl, tr, thickness, colour);
    render_draw_line(r, tr, br, thickness, colour);
    render_draw_line(r, br, bl, thickness, colour);
    render_draw_line(r, bl, tl, thickness, colour);
}

void render_draw_circle(Renderer *r, NES_Vector2 centre, float radius, NES_Vector4 colour) {
    //TODO:Change this so we at least draw some of the triangles this batch and the rest in the next one
    if(GET_ATLAS_BATCH(r, 0).vertex_count + CIRCLE_LINE_SEGMENTS + 1 >= MAX_VERTICIES || GET_ATLAS_BATCH(r, 0).index_count + (CIRCLE_LINE_SEGMENTS * 3) >= MAX_INDECIES) {
        render_next_atlas_batch(r, 0);
    }

    uint32_t center_index = GET_ATLAS_BATCH(r, 0).vertex_count;
    GET_ATLAS_BATCH(r, 0).vertex_data[GET_ATLAS_BATCH(r, 0).vertex_count++] = (Render_Vertex){centre, colour};

    float angle_step = 2.0f * M_PI / CIRCLE_LINE_SEGMENTS;
    uint32_t ring_start = GET_ATLAS_BATCH(r, 0).vertex_count;

    //Generating the vertices for the triangles that make up a circle
    for(int i = 0; i < CIRCLE_LINE_SEGMENTS; i++) {
        float angle = i * angle_step;
        float x = centre.x + cosf(angle) * radius;
        float y = centre.y - sinf(angle) * radius;

        GET_ATLAS_BATCH(r, 0).vertex_data[GET_ATLAS_BATCH(r, 0).vertex_count++] = (Render_Vertex){(NES_Vector2){x, y}, colour};
    }

    //Generating the indecies for the triangle ebo
    for(int i = 0; i < CIRCLE_LINE_SEGMENTS; i++) {
        uint32_t current = ring_start + i;
        uint32_t next = ring_start + ((i+1) % CIRCLE_LINE_SEGMENTS);

        GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = center_index;
        GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = current;
        GET_ATLAS_BATCH(r, 0).index_data[GET_ATLAS_BATCH(r, 0).index_count++] = next;
    }
}

