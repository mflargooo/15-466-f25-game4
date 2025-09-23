#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ScreenSpaceColorTextureProgram.hpp"
#include "StoryGraph.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>


// GLuint main_meshes_for_color_texture_program = 0;
/* Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("main.pnct"));
	// main_meshes_for_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
}); */

Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("main.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		// for the camera.
	});
});

PlayMode::PlayMode() : scene(*main_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	font_rasterizers.try_emplace("willy", data_path("../fonts/windsol.ttf"), 48);
	font_rasterizers.at("willy").set_line_spacing(4.f);

	char ascii[94];
	for (char i = 0; i < 94; i++) {
		ascii[i] = i + 32; 
	}
	font_rasterizers.at("willy").register_alphabet_to_texture(ascii, 94, 2048);

	story_graph.init("./late_for_work.txt");
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_W || evt.key.key == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			story_graph.prev_selection();
			return true;
		} else if (evt.key.key == SDLK_S || evt.key.key == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			story_graph.next_selection();
			return true;
		} else if (evt.key.key == SDLK_SPACE || evt.key.key == SDLK_RETURN) {
			story_graph.transition();
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	/* if have time to add sounds on transitions

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}*/

	//reset button press counters:
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	/*
	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);
	*/

	glClearColor(0.003f, 0.003f, 0.005f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	// scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));
	}
	glm::vec2 pen = glm::vec2(100.f, 150.f);
	story_graph.render_scene(font_rasterizers.at("willy"), pen);
	GL_ERRORS();
}
