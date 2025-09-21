#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

//Shader program that draws transformed, vertices tinted with vertex colors:
struct ScreenSpaceColorTextureProgram {
	ScreenSpaceColorTextureProgram();
	~ScreenSpaceColorTextureProgram();

	GLuint program = 0;
	//Attribute (per-vertex variable) locations:
	GLuint Position_vec2 = -1U;
	GLuint TexCoord_vec2 = -1U;
	//Uniform (per-invocation variable) locations:
	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< ScreenSpaceColorTextureProgram > screen_space_color_texture_program;