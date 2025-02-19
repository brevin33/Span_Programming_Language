#include "scope.h"

optional<Variable*> Scope::getVariableFromName(const string& name) {
    auto t = nameToVariable.find(name);
    if (nameToVariable.end() == t) {
        if (parent == nullptr) return nullopt;
        return parent->getVariableFromName(name);
    }
    return &nameToVariable[name];
}

bool Scope::addVariable(const Variable& var) {
    auto t = nameToVariable.find(var.name);
    if (nameToVariable.end() == t) {
        nameToVariable[var.name] = var;
        return true;
    }
    return false;
}
