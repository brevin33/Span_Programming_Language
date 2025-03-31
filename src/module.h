#pragma once
#include "utils.h"
#include "token.h"
#include "function.h"


class Module {
public:
    Module(const string& dir);
    void findStarts();


public:
    // makes accessing tokens easier
    vector<int> functionStarts;
    unordered_map<string, functionPrototype> functionPrototypes;
    LLVMModuleRef llvmModule;
    string dir;
    int tokensStart;
    int tokensEnd;
    bool hasMain = false;
};
bool looksLikeType();
bool looksLikeFunction();
bool looksLikeTemplateNames();
bool looksLikeTemplateTypes();
