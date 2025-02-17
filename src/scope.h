#pragma once
#include "utils.h"
#include "variable.h"

class Scope {
public:
public:
    unordered_map<string, Variable> nameToVariable;
    unique_ptr<Scope> parent = nullptr;

private:
private:
};