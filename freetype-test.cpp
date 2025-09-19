#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <vector>

//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

int main(int argc, char **argv) {
	constexpr size_t FONT_SIZE = 24;
	constexpr FT_UInt RESOLUTION = 32;

	const char *fontfile = "fonts/windsol.ttf";
	// const char *text = "test";

	FT_Library library;
	FT_Init_FreeType( &library );

	FT_Face ft_face;
	FT_New_Face(library, fontfile, 0, &ft_face);
	FT_Set_Char_Size(ft_face, FONT_SIZE * 64 * 2, FONT_SIZE * 64, RESOLUTION, RESOLUTION);

	hb_font_t *font = hb_ft_font_create(ft_face, NULL);

	hb_buffer_t *buf = hb_buffer_create();
	/*
	hb_buffer_add_utf8(buf, text, -1, 0, -1);
	hb_buffer_guess_segment_properties(buf);

	hb_shape(font, buf, NULL, 0);

	unsigned int num_glyphs = hb_buffer_get_length(buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, NULL);

	std::cout << "Buf contents" << std::endl;
	for (unsigned int i = 0; i < num_glyphs; i++) {
		hb_codepoint_t gid = info[i].codepoint;
		unsigned int cluster = info[i].cluster;
		double x_adv = pos[i].x_advance / 64;
		double y_adv = pos[i].y_advance / 64;
		double x_offset = pos[i].x_offset / 64;
		double y_offset = pos[i].y_offset / 64;

		char glyphname[32];
		hb_font_get_glyph_name(font, gid, glyphname, sizeof(glyphname));

		std::cout << "glyph=" << glyphname;
		std::cout << " cluster=" << cluster;
		std::cout << " advance=" << x_adv << ", " << y_adv << std::endl;
		std::cout << " offset=" << x_offset << ", " << y_offset << std::endl;
	}
	hb_buffer_clear_contents(buf);
	*/

	/* maybe work this into a function:
	hb_buffer_add_utf8(buf, "Hello, world!", -1, 0, -1);
	hb_buffer_guess_segment_properties(buf);

	hb_shape(font, buf, NULL, 0);

	num_glyphs = hb_buffer_get_length(buf);
	info = hb_buffer_get_glyph_infos(buf, NULL);
	pos = hb_buffer_get_glyph_positions(buf, NULL);


	unsigned char min_pixel_intensity = 50;
	for (unsigned int i = 0; i < num_glyphs; i++) {
		FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_RENDER);
		FT_Bitmap *bm = &ft_face->glyph->bitmap;

		for (unsigned int r = 0; r < bm->rows; r++) {
			for (unsigned int c = 0; c < bm->width; c++) {
				std::cout << (bm->buffer[c + r * bm->width] > min_pixel_intensity ? "#" : " ");
			}
			std::cout << std::endl;
		}
	}
	*/

	const char *hello_world = "Hello, world!";
	hb_buffer_add_utf8(buf, hello_world, -1, 0, -1);
	hb_buffer_guess_segment_properties(buf);

	hb_shape(font, buf, NULL, 0);

	unsigned int num_glyphs = hb_buffer_get_length(buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, NULL);

	// printing inline, so gather rows and sum columns
	std::vector< unsigned int >rows(num_glyphs);
	std::vector< unsigned int >widths(num_glyphs);
	std::vector< signed int >spacing(num_glyphs);

	std::vector< unsigned int >width_scan(num_glyphs);
	unsigned int max_rows = 0;
	unsigned int total_width = 0;

	for (unsigned int i = 0; i < num_glyphs; i++) {
		FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_RENDER);
		rows[i] = ft_face->glyph->bitmap.rows;
		max_rows = std::max(max_rows, rows[i]);

		widths[i] = ft_face->glyph->bitmap.width;
		spacing[i] = ft_face->glyph->bitmap_left;

		if (i > 0)
			width_scan[i] = widths[i] + width_scan[i-1];

		total_width += widths[i] + spacing[i];
	}

	unsigned char min_pixel_intensity = 128;
	for (unsigned int r = 0; r < max_rows; r++) {
		for (unsigned int i = 0; i < num_glyphs; i++) {
			FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_RENDER);
			FT_Bitmap *bm = &ft_face->glyph->bitmap;

			for (unsigned int c = 0; c < bm->width; c++) {
				if (max_rows - r > bm->rows) {
					std::cout << " ";
				}
				else {
					std::cout << (bm->buffer[c + (r - max_rows + bm->rows) * bm->width] > min_pixel_intensity ? "#" : " ");
				}
			}

			if (hello_world[info[i].cluster] == ' ') {
				for (hb_position_t c = 0; c < pos[i].x_advance / 64; c++) {
					std::cout << " ";
				}
			}
		}
		std::cout << std::endl;
	}

	/*
	for (unsigned int i = 0; i < num_glyphs; i++) {
		FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_RENDER);
		FT_Bitmap *bm = &ft_face->glyph->bitmap;

		for (unsigned int r = 0; r < bm->rows; r++) {
			for (unsigned int c = 0; c < bm->width; c++) {
				std::cout << (bm->buffer[c + r * bm->width] > min_pixel_intensity ? "#" : " ");
			}
			std::cout << std::endl;
		}
	}
	*/





	hb_buffer_destroy(buf);
  	hb_font_destroy(font);
	FT_Done_Face(ft_face);
	FT_Done_FreeType(library);
}
