#include "scope.h"
#include "span.h"

Scope::Scope(Scope* parent, LLVMBasicBlockRef start) {
    this->parent = parent;
    blocks.push_back(start);
}

Scope::Scope() {
}

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

void Scope::addBlock(LLVMBasicBlockRef block) {
    blocks.push_back(block);
}

void Scope::gotoLast() {
    LLVMBuildBr(builder, blocks.back());
}
