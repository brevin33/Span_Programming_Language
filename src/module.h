#pragma once
#include "utils.h"
#include "tokens.h"

class Module {
public:
    Module(string dir);
    vector<Module> dependecies;
    string name;
    Tokens tokens;
};
