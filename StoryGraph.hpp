#pragma once

#include "read_write_chunk.hpp"
#include "FontRast.hpp"

#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <variant>
#include <functional>

struct StoryGraph {
    struct Node {

        struct Choice {
            std::string text;
            std::string to_state;
            std::vector< std::function< void() >> on_select;
            std::vector< std::function< bool() >> render_conditions;
            bool hide = false;
        };

        std::string text;
        size_t text_length = 0;

        // choice dialogue to state name
        std::vector < Choice > choices;

        // selection in range { 0, ..., choices.size() - 1 }
        size_t selection = 0;

        Node() = default;
    };

    void init(const char *filename);
    Node *state = nullptr;

    std::map < std::string, Node > nodes;
    std::map < std::string, std::variant< int, std::string, bool >> variables;

    // anchor is top-left
    void render_scene(FontRast &rasterizer, glm::vec2 &anchor);
    void prev_selection();
    void next_selection();
    void transition();

    private:
        bool has_same_type(std::string var, std::string val);
        void debug_print();
};