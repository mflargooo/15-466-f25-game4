#include "ScreenSpaceColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

#include <iostream>

Load< ScreenSpaceColorTextureProgram > screen_space_color_texture_program(LoadTagEarly);

ScreenSpaceColorTextureProgram::ScreenSpaceColorTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"in vec2 Position;\n"
		"in vec2 TexCoord;\n"
		"out vec2 texCoord;\n"
		"uniform mat4 Ortho;\n"
		"void main() {\n"
		"	gl_Position = Ortho * vec4(Position, 0.f, 1.f);\n"
		"	texCoord = TexCoord;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform vec3 Color;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = vec4(Color, texture(TEX, texCoord).r)\n;"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec2 = glGetAttribLocation(program, "Position");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ScreenSpaceColorTextureProgram::~ScreenSpaceColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

