#pragma once
#include "utils.h"
#include "token.h"


class Module {
public:
    Module(const string& dir);
    void findStarts();

    bool looksLikeType();
    bool looksLikeFunction();

public:
    // makes accessing tokens easier
    const vector<Token>& tokens;
    vector<int> functionStarts;
    LLVMModuleRef llvmModule;
    string dir;
    int tokensStart;
    int tokensEnd;
    int t;  // tokenPositon
};