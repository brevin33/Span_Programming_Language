#pragma once
#include "utils.h"
#include "token.h"
#include "function.h"
#include "scope.h"

class Function;

class Module {
public:
    Module(const string& dir);

    ~Module();

    void loadTokens();

    void findStarts();

    void setupTypesAndFunctions();

    void logError(const string& err, Token* token = nullptr, bool wholeLine = false);

    void printResult();

public:
    LLVMModuleRef llvmModule;

private:
    string dir;
    vector<string> files;
    vector<vector<string>> textByFileByLine;
    vector<TokenPositon> functionStarts;
    vector<Module*> moduleDeps;
    Tokens tokens;
    bool hadError = false;

private:
    optional<Type> typeFromTokens();

    optional<Value> parseStatment(const vector<TokenType>& del, Scope& scope, int prio = INT_MIN);

    optional<Value> parseValue(Scope& scope);

    optional<Value> Module::parseFunctionCall(string& name, Scope& scope);

    Function* prototypeFunction(TokenPositon start);

    void implementFunction(TokenPositon start, Function& func);

    void implementScope(TokenPositon start, Scope& scope, Function& func);

    bool looksLikeType();

    bool looksLikeFunction();
};