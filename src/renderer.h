#ifndef __RENDERER_H__
#define __RENDERER_H__
#include <stdint.h>
#include <stddef.h>

//TODO: When spinning this out into its own library, need to add an arena to
//      manage the total memory easily
typedef float mat4[4][4];

typedef struct {
    uint8_t *pixel_buf; //Array of pixel data
    size_t buf_sz;
    size_t width, height;
    uint32_t pbo; //pbo this renderer uses
    uint32_t pixel_tex; //The texture the pbo is rendered to
} PixelBuffer;

//Creates a pixel buffer to hold the pixels representing
//a texture of size width * height
//Pixel size should be either 3 or 4 (rgb/rgba)
PixelBuffer pixelbuffer_init(size_t width, size_t height, uint8_t pixel_size);
//Frees the data used by a pixel buffer
void pixelbuffer_free(PixelBuffer *pb);

//Arbitrary constants for now
#define MAX_TRIANGLES 2048
#define MAX_QUADS 4096
#define MAX_LAYERS 64

#define VERTICIES_PER_QUAD 4
#define VERTICIES_PER_TRIANGLE 3
#define INDECIES_PER_QUAD 6
#define INDECIES_PER_TRIANGLE 3
#define INITIAL_VERTEX_CAPACITY MAX_QUADS * VERTICIES_PER_QUAD + MAX_TRIANGLES * VERTICIES_PER_TRIANGLE
#define INITIAL_INDEX_CAPACITY MAX_QUADS * INDECIES_PER_QUAD + MAX_TRIANGLES * INDECIES_PER_TRIANGLE
#define INVALID_TEX_INDEX 1248
#define CIRCLE_LINE_SEGMENTS 64 //Number of line segments that make up the circumference of a circle

typedef struct {
    float x;
    float y;
    float w;
    float h;
} NES_Quad;

typedef struct {
    float x, y;
} NES_Vector2;

typedef struct {
    float x, y, z, w;
} NES_Vector4;

//Data structure to hold data about a single render vertex
typedef struct {
    NES_Vector2 pos; //The on-screen position of the render vertex
    NES_Vector4 colour; //The colour of the vertex
    NES_Vector2 uv; //The (u,v) coordinates of the vertex
} Render_Vertex;

typedef struct {
    uint32_t texture; //GL index of the atlas texture
    uint32_t width, height; //Dimensions of the atlas
    uint32_t cursor_x, cursor_y; //Current packing position
    uint32_t row_height; //Height of the tallest texture on the current row
    uint8_t pixel_size; //Number of bytes a pixel in the atlas takes up. Must be either 3 or 4
} TextureAtlas;

//Initialises a texture atlas
//Optionally packs a single white pixel at the start of the texture atlas to render a solid quad
TextureAtlas atlas_init(uint32_t width, uint32_t height, uint32_t texture, uint8_t pixel_size);
TextureAtlas atlas_init_blank(uint32_t width, uint32_t height, uint8_t pixel_size);
//Returns the UV rect where the texture was placed
//pixel_size must be either 3 or 4. If it does not match the pixel size of the atlas,
//an empty quad will returned as the pixel formats are different
NES_Quad atlas_pack(TextureAtlas *a, uint8_t* pixels, size_t w, size_t h, uint8_t pixel_size);
void atlas_free(TextureAtlas *a);

//Represents a single batch sent off in a draw call from a texture atlas
typedef struct {
    Render_Vertex *vertex_data;
    uint32_t *index_data; //The index count (for ebo) for this renderer
    TextureAtlas *a;
    size_t vertex_size;
    size_t index_size;
    size_t vertex_count;
    size_t index_count;
} AtlasRenderBatch;

typedef struct {
    AtlasRenderBatch *atlas_batches;
    int earliest_atlas_used; //If negative, this layer is not used, otherwise is the index of the earliest atlas used
} Render_Layer;

//Pixel renderer that renders a single frame
typedef struct {
    Render_Layer layers[MAX_LAYERS]; //Layers of rendering
    mat4 projection; //projection matrix for this renderer
    uint32_t vao; //vao this renderer uses
    uint32_t vbo; //vbo this renderer uses
    uint32_t ebo; //ebo this renderer uses
    uint32_t shader; //shader this renderer uses
    size_t atlas_batch_capacity;
    size_t num_atlas_batches;

    uint32_t screen_height;
    uint32_t screen_width;
} Renderer;

#define GET_ATLAS_BATCH(r, layer, i) (r)->layers[layer].atlas_batches[i]

//Initialises the pixel renderer
Renderer render_init(size_t width, size_t height);
//Frees a pixel renderer
void render_free(Renderer *r);
//Sets up the variables for renderering to the pbo from the Renderer
void render_begin(Renderer *r);
//Ends rendering to the current pixel frame
void render_end(Renderer *r);
//Adds a texture atlas to the render's context and returns a reference to
//use the texture atlas by
//TODO: Add a way to return a failure value to this
//      Add a way to remove an atlas efficiently
uint32_t add_texture_atlas(Renderer *r, TextureAtlas *ta);
//Draws a quad
void render_draw_atlas_quad(Renderer *r, NES_Quad dimensions, NES_Quad uv_dimensions, NES_Vector4 colour, uint32_t atlas, uint8_t layer);
//Draws a dynamically allocated texture
void render_draw_texture(Renderer *r, uint32_t texture, NES_Quad dimensions, NES_Quad uv_dimensions, NES_Vector4 colour);
//Draws a frame straight to a texture by uploading it to a pixel buffer
void pixelbuffer_updload_data(PixelBuffer *pb, uint8_t *data);
//Draws a pixel buffer
//TODO: Figure out how to add a layering system to drawing pixelbuffers/dynamic textures in general
void render_draw_pixel_buffer(Renderer *r, PixelBuffer *pb);
//Draws a filled circle
void render_draw_circle(Renderer *r, NES_Vector2 centre, float radius, NES_Vector4 colour, uint8_t layer);
//Draws a filled quad
void render_draw_quad(Renderer *r, NES_Quad quad, NES_Vector4 colour, uint8_t layer);
//TODO:Draws an unfilled circle
void render_draw_unfilled_circle(Renderer *r, NES_Vector2 centre, float radius, NES_Vector4 colour, uint8_t layer);
//Draws an unfilled quad
void render_draw_unfilled_quad(Renderer *r, NES_Quad quad, float thickness, NES_Vector4 colour, uint8_t layer);
//Draws a line between two points
void render_draw_line(Renderer *r, NES_Vector2 start_pos, NES_Vector2 end_pos, float thickness, NES_Vector4 colour, uint8_t layer);
void render_draw_quad_bordered(Renderer *r, NES_Quad quad, NES_Vector4 q_col, NES_Vector4 b_col, float thick, uint8_t layer);

//Determines the projection matrix
void ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);

#endif //__RENDERER_H__
