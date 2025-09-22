#include "StoryGraph.hpp"
#include "FontRast.hpp"
#include "data_path.hpp"
#include <iostream>
#include <fstream>

// trim by https://cplusplus.com/forum/beginner/251052/
std::string trim(std::string s) {
    size_t start = s.find_first_not_of("\t\v\r\n");
    size_t end = s.find_last_not_of("\t\v\r\n");
    
    if (end == start) return "";
    return s.substr(start, end - start + 1);
}

// init by https://stackoverflow.com/questions/7868936/read-file-line-by-line-using-ifstream-in-c
void StoryGraph::init(const char *filename) {
    std::ifstream file(data_path(filename), std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + std::string(filename));
    }

    // first check if there are any defined variables
    std::string line;

    bool def_block = false;
    Node *curr = nullptr;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.size() == 0) continue;
    
        if (line == "begindef") {
            def_block = true;
        }
        else if (line == "enddef") {
            def_block = false;
        }

        if (def_block) {
            size_t split_at = line.find("=");
            if (split_at == std::string::npos) {
                std::cout << "Invalid variable definition" << std::endl;
            }
            else {
                std::string var = trim(line.substr(0, split_at));
                std::string val = trim(line.substr(split_at + 1));
                if (variables.find(var) != variables.end()) {
                    std::cout << "Variable " << var << " already defined" << std::endl;
                    continue;
                }

                variables.try_emplace(var, val);
            }

            continue;
        }

        if (line[0] == '@') {
            if (line.size() == 1) continue;

            if (nodes.find(line.substr(1)) == nodes.end()) {            
                nodes.try_emplace(line.substr(1));
            }

            curr = &nodes.at(line.substr(1));
        }
        else if (line.substr(0, 6) == "%text=" && curr) {
            curr->text = line.substr(6);
            curr->text_length = line.size() - 6;
        }
        else if (line[0] == '$' && curr) {
            if (line.substr(1, 7) == "choice=") {
                size_t split_at = line.find("|");
                std::string choice_text = line.substr(8, split_at - 8);
                std::string transition_state = line.substr(split_at + 1); 

                curr->choices.emplace_back(std::pair(choice_text, trim(transition_state)));
            }
            else if (line.substr(1, 3) == "end") {
                std::cout << "Reached end of branch" << std::endl;
            }
        }
    }
    
    if (nodes.find("init") == nodes.end()) {
        throw std::runtime_error("StoryGraph could not find initial state: init");
    }

    state = &nodes.at("init");
    assert(state);

    // debug_print();
}

void StoryGraph::debug_print() {
    for (auto [st, node] : nodes) {
        std::cout << "@: " <<  st << std::endl;
        std::cout << node.text << std::endl;

        for (auto [choice, transition_state] : node.choices) {
            std::cout << "choice: " << choice << std::endl << "to: " << transition_state << std::endl;
        }

        std::cout << std::endl;
    }
}

void StoryGraph::render_scene(FontRast &rasterizer, glm::vec2 &anchor) {
    glm::vec2 local = anchor;
    rasterizer.raster_text(state->text.data(), state->text_length, glm::u8vec3(0), local);

    // reposition anchor for choices
    local.x = anchor.x + 50.f; //50 pixel indent
    local.y += + 20.f;
    
    for (size_t i = 0; i < state->choices.size(); i++) {
        auto p = state->choices[i];
        rasterizer.raster_text(p.first.data(), p.first.length(), i == state->selection ? glm::u8vec3(255, 255, 0) : glm::u8vec3(0), local);
    }

    anchor = local;
}