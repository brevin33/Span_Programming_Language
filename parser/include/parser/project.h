#pragma once
#include "parser/tokens.h"
#include <string>

struct Project {
    Project(std::string folder);
    ~Project() {
    }
    std::string folder;
    std::vector<Tokens> tokens;
};
