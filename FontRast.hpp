#pragma once

#include "GL.hpp"
#include "Scene.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>
#include <map>
#include <list>
#include <glm/glm.hpp>

// single texture atlas per font inspired by https://www.youtube.com/watch?v=9ufh1rl7yMY

struct GlyphTexInfo {
    // first two coords represent top left u,v coords
    // second two represent bottom right u,v coords for
    // each character registered
    float u0, v0, u1, v1;
    FT_Int left;
    FT_Int top;
    FT_Vector advance;
    unsigned int rows;
    unsigned int width;
};

struct FontRast {
    // this function overwrites the current texture belonging to this
    // instance of FontRast---in other words adding more letters required
    // specifying the ENTIRE desired alphabet---expects no duplicates.
    //
    // texture is populate in the order of alphabet, and corresponding
    // u,v coordinates can be looked up
    // len = number of characters in alphabet to register to texture
    GLuint texture;
    void register_alphabet_to_texture(const char *alphabet, int len, GLsizei texture_size);
    GlyphTexInfo lookup(const unsigned char chr);
    void raster_text(const char *str, int len, glm::u8vec3 color, glm::vec2 abs_coords);
    
    FontRast(const char *fontfile, unsigned int pixel_height);
    
    private:
        struct Vertex {
            glm::vec2 Position;
            glm::vec2 TexCoord;
        };

        std::map< unsigned char, GlyphTexInfo > lookup_tex;
        FT_Library library;
        FT_Face ft_face;
        hb_buffer_t *buf;
        hb_font_t *font;

        // maintains vertex arrays per font
        GLuint program;
        GLuint vao, vbo;
};