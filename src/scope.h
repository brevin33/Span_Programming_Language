#pragma once
#include "utils.h"
#include "variable.h"

class Scope {
public:
    Scope(Scope* parent, LLVMBasicBlockRef start);
    Scope();
    optional<Variable*> getVariableFromName(const string& name);
    bool addVariable(const Variable& var);
    void gotoLast();
    void addBlock(LLVMBasicBlockRef block);

public:
    Scope* parent = nullptr;
    Scope* child = nullptr;
private:
    unordered_map<string, Variable> nameToVariable;
    vector<LLVMBasicBlockRef> blocks;

private:
};