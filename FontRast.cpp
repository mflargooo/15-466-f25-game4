#include "FontRast.hpp"
#include "GL.hpp"
#include "gl_errors.hpp"
#include "Scene.hpp"
#include "ScreenSpaceColorTextureProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
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
    GL_ERRORS();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 6, nullptr, GL_DYNAMIC_DRAW);
    GL_ERRORS();

    glBindVertexArray(vao);
    GL_ERRORS();
    
    // register attribs
    glEnableVertexAttribArray(screen_space_color_texture_program->Position_vec2);
    glEnableVertexAttribArray(screen_space_color_texture_program->TexCoord_vec2);

    glVertexAttribPointer(screen_space_color_texture_program->Position_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(FontRast::Vertex), (GLbyte *)0 + offsetof(FontRast::Vertex, Position));
    glVertexAttribPointer(screen_space_color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(FontRast::Vertex), (GLbyte *)0 + offsetof(FontRast::Vertex, TexCoord));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &texture);

    program = screen_space_color_texture_program->program;
    GL_ERRORS();
}

// gl subimage method for font texture management inspired by 
// https://stackoverflow.com/questions/55892503/freetype-texture-atlas-why-is-my-text-rendering-as-quad
void FontRast::register_alphabet_to_texture(const char *alphabet, int len, GLsizei texture_size) {
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
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
        if (ft_face->glyph->bitmap.width == 0 || ft_face->glyph->bitmap.rows == 0) continue;

        // avoid horizontal overflow
        if (tex_pen_x + ft_face->glyph->bitmap.width >= (unsigned int)texture_size) {
            tex_pen_x = 0;
            tex_pen_y += max_y_ofs;
        }
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

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
        tex_info.advance.x = ft_face->glyph->advance.x;
        tex_info.advance.y = ft_face->glyph->advance.y;

        lookup_tex[alphabet[i]] = tex_info;

        // advance pen
        tex_pen_x += ft_face->glyph->bitmap.width + 1;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    GL_ERRORS();
}

GlyphTexInfo FontRast::lookup(const unsigned char chr) {
    if (lookup_tex.find(chr) == lookup_tex.end()) {
        throw std::runtime_error("Character has not been registered to the texture.");
    }

    return lookup_tex[chr];
}

// len = -1 if string is null terminated;
void FontRast::raster_text(const char *str, int len, glm::u8vec3 color, glm::vec2 at) {
    hb_buffer_add_utf8(buf, str, len, 0, -1);
	hb_buffer_guess_segment_properties(buf);
	hb_shape(font, buf, NULL, 0);

	unsigned int num_glyphs = hb_buffer_get_length(buf);

	struct Viewport {
		GLint x;
		GLint y;
		GLint width;
		GLint height;
	} data;
	glGetIntegerv(GL_VIEWPORT, (GLint *)&data);
	// viewport 0, 0 is bottom-left -> top-left
	glm::mat4 Ortho = glm::ortho((float)data.x, (float)(data.x + data.width), (float)data.y, (float)(data.y + data.height),  -1.f, 1.f);

	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, NULL);
	
    glUseProgram(program);
	
    glUniformMatrix4fv(glGetUniformLocation(program, "Ortho"), 1, GL_FALSE, glm::value_ptr(Ortho));
    glUniform3f(glGetUniformLocation(program, "Color"), (float)color.x / 255.f, (float)color.y / 255.f, (float)color.z / 255.f);

    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (unsigned int i = 0; i < num_glyphs; i++) {
        GlyphTexInfo glyph = lookup(str[i]);

        float pen_x = at.x + pos[i].x_offset + glyph.left;
        float pen_y = at.y - pos[i].y_offset - glyph.top;

        Vertex vertices[6];

        // top-left triangle
        vertices[0].Position = glm::vec2(pen_x, pen_y);
        vertices[0].TexCoord = glm::vec2(glyph.u0, glyph.v0);

        vertices[1].Position = glm::vec2(pen_x + glyph.width, pen_y);
        vertices[1].TexCoord = glm::vec2(glyph.u1, glyph.v0);

        vertices[2].Position = glm::vec2(pen_x, pen_y + glyph.rows);
        vertices[2].TexCoord = glm::vec2(glyph.u0, glyph.v1);

        // bottom-right triangle
        vertices[3].Position = glm::vec2(pen_x + glyph.width, pen_y);
        vertices[3].TexCoord = glm::vec2(glyph.u1, glyph.v0);

        vertices[4].Position = glm::vec2(pen_x, pen_y + glyph.rows);
        vertices[4].TexCoord = glm::vec2(glyph.u0, glyph.v1);

        vertices[5].Position = glm::vec2(pen_x + glyph.width, pen_y + glyph.rows);
        vertices[5].TexCoord = glm::vec2(glyph.u1, glyph.v1);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        at.x += glyph.advance.x / 64;
        at.y += glyph.advance.y / 64;
        GL_ERRORS();
    }


    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);

    hb_buffer_clear_contents(buf);
    
    GL_ERRORS();
}