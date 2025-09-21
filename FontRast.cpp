#include "FontRast.hpp"
#include "GL.hpp"
#include "gl_errors.hpp"
#include "Scene.hpp"
#include "ColorTextureProgram.hpp"

#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>
#include <set>

// from the harfbuzz and freetype examples, partitioned into respective functions
// to reduce recomputation

// gl init code inspired by DrawLines, Mesh, and https://learnopengl.com/In-Practice/Text-Rendering 
FontRast::FontRast(const char *fontfile, unsigned int pixel_height) {
	FT_Init_FreeType(&library);
	FT_New_Face(library, fontfile, 0 /* make a system to allocate unsused indices */, &ft_face);
    FT_Set_Pixel_Sizes(ft_face, 0, pixel_height);

    font = hb_ft_font_create(ft_face, NULL);
    buf = hb_buffer_create();

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    
    // register attribs
    glVertexAttribPointer(color_texture_program->Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(FontRast::Vertex), (GLbyte *)0 + offsetof(FontRast::Vertex, Position));
    glVertexAttribPointer(color_texture_program->Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(FontRast::Vertex), (GLbyte *)0 + offsetof(FontRast::Vertex, Color));
    glVertexAttribPointer(color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(FontRast::Vertex), (GLbyte *)0 + offsetof(FontRast::Vertex, TexCoord));
	
    glEnableVertexAttribArray(color_texture_program->Position_vec4);
    glEnableVertexAttribArray(color_texture_program->Color_vec4);
    glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &texture);

    program = color_texture_program->program;
    GL_ERRORS();
}

// gl subimage method for font texture management inspired by 
// https://stackoverflow.com/questions/55892503/freetype-texture-atlas-why-is-my-text-rendering-as-quad
void FontRast::register_alphabet_to_texture(const char *alphabet, int len, GLsizei texture_size) {
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        texture_size,
        texture_size,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    // get height texture spacing
    GLint max_y_ofs = 0;
    for (size_t i = 0; i < len; i++) {
        if (FT_Load_Char(ft_face, alphabet[i], FT_LOAD_RENDER) != 0) {
            throw std::runtime_error("Could not load char " + alphabet[i]);
        }
        max_y_ofs = (GLint)std::max((int)max_y_ofs, (int)ft_face->glyph->bitmap.rows);
    }

    unsigned int tex_pen_x = 0, tex_pen_y = 0;
    for (size_t i = 0; i < len; i++) {
        if (FT_Load_Char(ft_face, alphabet[i], FT_LOAD_RENDER) != 0) {
            throw std::runtime_error("Could not load char " + alphabet[i]);
        }

        std::cout << alphabet[i] << ", ";

        // avoid horizontal overflow
        if (tex_pen_x + ft_face->glyph->bitmap.width >= (unsigned int)texture_size) {
            tex_pen_x = 0;
            tex_pen_y += max_y_ofs;
        }
        
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexSubImage2D(GL_TEXTURE_2D, 0, tex_pen_x, tex_pen_y, 
            ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows, 
            GL_RED, GL_UNSIGNED_BYTE, ft_face->glyph->bitmap.buffer);
        

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // store glyph information for lookup
        GlyphTexInfo tex_info;
        tex_info.u0 = (float)tex_pen_x / (float)texture_size; 
        tex_info.v0 = (float)tex_pen_y / (float)texture_size;
        tex_info.u1 = (float)(tex_pen_x + ft_face->glyph->bitmap.width) / (float)texture_size;
        tex_info.v1 = (float)(tex_pen_y + ft_face->glyph->bitmap.rows) / (float)texture_size;

        tex_info.left = ft_face->glyph->bitmap_left;
        tex_info.top = ft_face->glyph->bitmap_top;
        tex_info.rows = ft_face->glyph->bitmap.rows;
        tex_info.width = ft_face->glyph->bitmap.width;
        tex_info.advance = ft_face->glyph->advance;

        lookup_tex[alphabet[i]] = tex_info;

        // advance pen
        tex_pen_x += ft_face->glyph->bitmap.width + 1;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    GL_ERRORS();
}

// len = -1 if string is null terminated;
void FontRast::raster_text(const char *str, int len, glm::u8vec4 color, glm::vec2 at) {
    hb_buffer_add_utf8(buf, str, len, 0, -1);
	hb_buffer_guess_segment_properties(buf);
	hb_shape(font, buf, NULL, 0);

	unsigned int num_glyphs = hb_buffer_get_length(buf);
	//hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, NULL);
	
    glUseProgram(program);
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    for (unsigned int i = 0; i < num_glyphs; i++) {
        GlyphTexInfo glyph = lookup(str[i]);

        float pen_x = at.x + glyph.left;
        float pen_y = at.y - glyph.top - glyph.rows;


        Vertex vertices[6];

        // top-left triangle
        vertices[0].Position = glm::vec3(pen_x, pen_y, 0.f);
        vertices[0].TexCoord = glm::vec2(glyph.u0, glyph.v0);
        vertices[0].Color = color;

        vertices[1].Position = glm::vec3(pen_x + glyph.width, pen_y, 0.f);
        vertices[1].TexCoord = glm::vec2(glyph.u1, glyph.v0);
        vertices[1].Color = color;

        vertices[2].Position = glm::vec3(pen_x, pen_y + glyph.rows, 0.f);
        vertices[2].TexCoord = glm::vec2(glyph.u0, glyph.v1);
        vertices[2].Color = color;

        // bottom-right triangle
        vertices[3].Position = glm::vec3(pen_x + glyph.width, pen_y, 0.f);
        vertices[3].TexCoord = glm::vec2(glyph.u1, glyph.v0);
        vertices[3].Color = color;

        vertices[4].Position = glm::vec3(pen_x, pen_y + glyph.rows, 0.f);
        vertices[4].TexCoord = glm::vec2(glyph.u0, glyph.v1);
        vertices[4].Color = color;

        vertices[5].Position = glm::vec3(pen_x + glyph.width, pen_y + glyph.rows, 0.f);
        vertices[5].TexCoord = glm::vec2(glyph.u1, glyph.v1);
        vertices[5].Color = color;

        pen_x += glyph.advance.x / 64;

        std::cout << str[i] << std::endl;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    hb_buffer_clear_contents(buf);
    
    GL_ERRORS();
}

GlyphTexInfo FontRast::lookup(const unsigned char chr) {
    if (lookup_tex.find(chr) == lookup_tex.end()) {
        throw std::runtime_error("Character has not been registered to the texture.");
    }

    return lookup_tex[chr];
}