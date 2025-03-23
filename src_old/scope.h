#pragma once
#include "utils.h"
#include "variable.h"

class Scope {
public:
    Scope(Scope* parent, LLVMBasicBlockRef start, bool canBreak);
    Scope();
    optional<Variable*> getVariableFromName(const string& name);
    bool addVariable(const Variable& var);
    void gotoLast();
    void gotoFront();
    void addBlock(LLVMBasicBlockRef block);

public:
    Scope* parent = nullptr;
    Scope* child = nullptr;
    vector<LLVMBasicBlockRef> blocks;
    bool canBreak;

private:
    unordered_map<string, Variable> nameToVariable;

private:
};