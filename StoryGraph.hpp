#pragma once

#include "read_write_chunk.hpp"
#include "FontRast.hpp"

#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct StoryGraph {
    struct Node {
        std::string text;
        size_t text_length = 0;

        // choice dialogue to state name
        std::vector < std::pair < std::string, std::string >> choices;
        // selection in range { 0, ..., choices.size() - 1 }
        size_t selection = 0;

        Node() = default;
    };

    void init(const char *filename);
    Node *state = nullptr;

    std::map < std::string, Node > nodes;
    std::map < std::string, std::string > variables;

    // anchor is top-left
    void render_scene(FontRast &rasterizer, glm::vec2 &anchor);

    private:
        void debug_print();
};