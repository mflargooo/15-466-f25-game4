#include "StoryGraph.hpp"
#include "FontRast.hpp"
#include "data_path.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>

void push_quit() {
    SDL_Event evt = {};
    evt.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&evt);
}
void push_keydown(SDL_Keycode code) {
    SDL_Event evt = {};
    evt.type = SDL_EVENT_KEY_DOWN;
    evt.key.key = code;
    SDL_PushEvent(&evt);
}

// trim by https://cplusplus.com/forum/beginner/251052/ but modified to handle one sided
// whitespace case
std::string trim(std::string s) {
    size_t start = s.find_first_not_of("\t\v\r\n ");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of("\t\v\r\n ");
    return s.substr(start, end - start + 1);
}

bool StoryGraph::has_same_type(std::string var, std::string val) {
    if (variables.find(var) == variables.end()) {
        std::cout << "Variable " << var << " not defined" << std::endl;
        return false;
    }                        
    std::variant< int, std::string, bool > &p = variables[var];
    
    //check integer
    bool is_int = true;
    try {
        int i = std::stoi(val);
        i;
    }
    catch (const std::invalid_argument &) {
        is_int = false;
    }
    if ((std::holds_alternative<int>(p) && !is_int) ||
    (std::holds_alternative<bool>(p) && (val != "true" && val != "false"))) {
        std::cout << "Invalid assignemnt: type mismatch" << std::endl;
        return false;
    }

    return true;
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
            size_t datatype_at = line.find_first_of(' ');
            size_t equal_at = line.find_last_of('=');
            if (datatype_at == std::string::npos || equal_at == std::string::npos) {
                std::cout << "Invalid variable definition" << std::endl;
            }
            else {
                std::string datatype = trim(line.substr(0, datatype_at));
                std::string var = trim(line.substr(datatype_at + 1, equal_at - datatype_at - 1));
                std::string val = trim(line.substr(equal_at + 1));

                std::cout << datatype << std::endl << var << std::endl << val << std::endl;;
                if (variables.find(var) != variables.end()) {
                    std::cout << "Variable " << var << " already defined" << std::endl;
                    continue;
                }
                
                if (datatype == "int") {
                    variables.try_emplace(var, std::stoi(val));
                }
                else if (datatype == "string") {
                    variables.try_emplace(var, val);
                }
                else if (datatype == "bool") {
                    if (val != "true" && val != "false") { 
                        throw std::runtime_error("Incorrect bool value: " + val); 
                    }
                    variables.try_emplace(var, val == "true");
                }
                else {
                    std::cout << "Invalid datatype: " << datatype << std::endl;
                }
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
                size_t first_pipe = line.find_first_of("|");
                size_t last_pipe = line.find_last_of("|");
                std::string choice_text = trim(line.substr(8, first_pipe - 8));
                std::string actions = first_pipe != last_pipe ? trim(line.substr(first_pipe + 1, last_pipe - first_pipe - 1)) : "";
                std::string transition_state = trim(line.substr(last_pipe + 1)); 

                curr->choices.emplace_back();
                Node::Choice &choice = curr->choices.back();
                choice.text = choice_text;
                choice.to_state = transition_state;

                // iterate through conditions
                size_t curr_cond = 0;
                size_t semi_at;
                while ((semi_at = actions.find_first_of(";", curr_cond)) != std::string::npos) {
                    std::string action = trim(actions.substr(curr_cond, semi_at - curr_cond));
                    std::cout << action << std::endl;
                    if (action.substr(0, 3) == "add") {
                        size_t space_at = action.find_first_of(' ', 4);
                        std::string var = trim(action.substr(4, space_at - 4));
                        std::string val = trim(action.substr(space_at + 1));

                        std::cout << "add, " << var << ", " << val << std::endl;
                        
                        if (variables.find(var) == variables.end()) {
                            std::cout << "Variable " << var << " not defined" << std::endl;
                            curr_cond = semi_at + 1;
                            continue;
                        }                        

                        auto *p = &variables[var];
                        if (!std::holds_alternative<int>(*p)) {
                            std::cout << "Operation add expects variable of datatype int" << std::endl;
                            curr_cond = semi_at + 1;
                            continue;
                        }

                        int amt = std::stoi(val);
                        choice.on_select.emplace_back([p, amt]() {
                            int *q = std::get_if<int>(p);
                            *q += amt;
                        });
                    }
                    else if (action.substr(0, 3) == "sub") {
                        size_t space_at = action.find_first_of(' ', 4);
                        std::string var = trim(action.substr(4, space_at - 4));
                        std::string val = trim(action.substr(space_at + 1));
                        
                        std::cout << "sub, " << var << ", " << val << std::endl;

                        if (variables.find(var) == variables.end()) {
                            std::cout << "Variable " << var << " not defined" << std::endl;
                            curr_cond = semi_at + 1;
                            continue;
                        }      

                        auto *p = &variables[var];
                        if (!std::holds_alternative<int>(*p)) {
                            std::cout << "Operation add expects variable of datatype int" << std::endl;
                            curr_cond = semi_at + 1;
                            continue;
                        }

                        int amt = std::stoi(val);
                        choice.on_select.emplace_back([p, amt]() {
                            int *q = std::get_if<int>(p);
                            *q -= amt;
                        });
                    }
                    else if (action.substr(0, 3) == "set") {
                        size_t space_at = action.find_first_of(' ', 4);
                        std::string var = trim(action.substr(4, space_at - 4));
                        std::string val = trim(action.substr(space_at + 1));
                        
                        if (!has_same_type(var, val)) {
                            std::cout << "Invalid operation: type mismatch" << std::endl;
                            curr_cond = semi_at + 1;
                            continue;
                        }

                        std::variant< int, std::string, bool > &p = variables[var];
                        choice.on_select.emplace_back([&p, val]() {
                            if (std::holds_alternative<int>(p)) {
                                p = std::stoi(val);
                            }
                            else if (std::holds_alternative<bool>(p)) {
                                p = (val == "true");
                            }
                            else {
                                p = val;
                            }
                        });
                    }
                    else if (action.substr(0, 7) == "show if") {
                        size_t space_at = action.find_first_of(' ', 8);
                        bool negate = trim(action.substr(8, space_at - 8)) == "not";
                        
                        std::cout << trim(action.substr(8, space_at - 8)) << std::endl;
                        
                        size_t start = 8;
                        if (negate) start += 4;

                        space_at = action.find_first_of(' ', start);
                        std::string var = trim(action.substr(start, space_at - start));
                        std::string op = trim(action.substr(space_at + 1, 1));
                        std::string val = trim(action.substr(space_at + 2, semi_at - space_at - 2));

                        std::cout << "show if " << (negate ? "not" : "") << var << ", " << op << ", " << val << std::endl;
                        if (!has_same_type(var, val)) {
                            std::cout << "Invalid operation: type mismatch" << std::endl;
                            curr_cond = semi_at;
                            continue;
                        }
                        else if (op != "=" && op != ">" && op != "<") {
                            std::cout << "Invalid operation: operator not defined" << std::endl;
                            curr_cond = semi_at;
                            continue;
                        }
                        
                        auto &p = variables[var];
                        if (!std::holds_alternative<int>(p) && (op == ">" || op == "<")) {
                            std::cout << "> and < operators are not defined for non-integers" << std::endl;
                            curr_cond = semi_at;
                            continue;
                        }

                        int parsed_int = 0;
                        bool parsed_bool = false;
                        if (std::holds_alternative<int>(p)){
                            parsed_int = std::stoi(val);
                        }
                        else if (std::holds_alternative<bool>(p)) {
                            parsed_bool = val == "true";
                        }
                        
                        if (std::holds_alternative<int>(p) && op == ">") {
                            choice.render_conditions.emplace_back([&p, parsed_int]() {
                                return std::get<int>(p) > parsed_int;
                            });
                        }
                        else if (std::holds_alternative<int>(p) && op == "<") {
                            choice.render_conditions.emplace_back([&p, parsed_int]() {
                                return std::get<int>(p) < parsed_int;
                            });
                        }
                        else {
                            choice.render_conditions.emplace_back([&p, val, parsed_int, parsed_bool, negate]() {
                                bool res = false;
                                if (auto *q = std::get_if<int>(&p)) {
                                    res = *q == parsed_int;
                                }
                                else if (auto *r = std::get_if<bool>(&p)) {
                                    res = *r == parsed_bool;                                    
                                }
                                else if (auto *s = std::get_if<std::string>(&p)) {
                                    res = *s == val;
                                }

                                return negate ? !res : res;
                            });
                        }
                    }

                    curr_cond = semi_at + 1;
                }
            }
            // simulating key pressed by https://stackoverflow.com/questions/35760351/simulate-keyboard-button-press-sdl-library
            else if (line.substr(1, 3) == "end") {
                curr->choices.emplace_back();
                Node::Choice &pa = curr->choices.back();
                pa.text = "Play Again";
                pa.to_state = "init";
                pa.on_select.emplace_back([]() {
                    push_keydown(SDLK_R);
                });

                curr->choices.emplace_back();
                Node::Choice &quit = curr->choices.back();
                quit.text = "Quit";
                quit.to_state = "init";
                quit.on_select.emplace_back([]() {
                    push_quit();
                });
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

void StoryGraph::render_scene(FontRast &rasterizer, glm::vec2 &anchor) {
    glm::vec2 local = anchor;
    rasterizer.raster_text(state->text.data(), state->text_length, glm::u8vec3(0), local);

    // reposition anchor for choices
    local.x = anchor.x + 50.f; //50 pixel indent
    local.y += + 20.f;
    
    for (size_t i = 0; i < state->choices.size(); i++) {
        auto &p = state->choices[i];
        bool render = true;

        if (p.render_conditions.size() > 0) {
            for (auto rc : p.render_conditions) {
                render = render && rc();
            }
        }
        
        if (render) {
            rasterizer.raster_text(p.text.data(), p.text.length(), i == state->selection ? glm::u8vec3(255, 255, 0) : glm::u8vec3(0), local);
        }

        p.hide = !render;
    }

    anchor = local;
}

void StoryGraph::prev_selection() {
    size_t select = state->selection;
    do {
        select = (select - 1 + state->choices.size()) % state->choices.size();
    } while (state->choices[select].hide);

    state->selection = select;
}

void StoryGraph::next_selection() {
    size_t select = state->selection;
    do {
        select = (select + 1) % state->choices.size();
    } while (state->choices[select].hide);

    state->selection = select;
}

void StoryGraph::transition() {
    size_t select = state->selection;

    for (auto func : state->choices[select].on_select) {
        func();
    }
    debug_print();
    state = &nodes[state->choices[select].to_state];
    if (state->choices[state->selection].hide) next_selection();
}

void StoryGraph::debug_print() {

    std::cout << "Variables: " << std::endl;
    for (auto [var, val] : variables) {
        std::visit([var](auto &&p) {
            std::cout << var << " = " << p << std::endl;
        }, val);
    }

    std::cout << std::endl;

    for (auto [st, node] : nodes) {
        std::cout << node.text << std::endl;

        for (auto choice : node.choices) {
            std::cout << "choice: " << choice.text << std::endl << "to: " << choice.to_state << std::endl;
        }

        std::cout << std::endl;
    }
}