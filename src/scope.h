#pragma once
#include "utils.h"
#include "variable.h"

class Scope {
public:
    optional<Variable*> getVariableFromName(const string& name);
    bool addVariable(const Variable& var);

public:
private:
    unordered_map<string, Variable> nameToVariable;
    unique_ptr<Scope> parent = nullptr;

private:
};