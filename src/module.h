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

    bool printResult();

    void compileToObjFile(const string& buildDir);

public:
    LLVMModuleRef llvmModule;

private:
    string dir;
    vector<string> files;
    vector<vector<string>> textByFileByLine;
    vector<TokenPositon> functionStarts;
    vector<TokenPositon> structStarts;
    vector<Module*> moduleDeps;
    unordered_map<string, TokenPositon> nameToStructStart;
    unordered_map<string, bool> nameToStructDone;
    Tokens tokens;
    bool hadError = false;
    bool hasMain = false;

private:
    optional<Type> typeFromTokens(bool logErrors = true);

    optional<Value> parseStatment(const vector<TokenType>& del, Scope& scope, int prio = INT_MIN);

    optional<Value> parseValue(Scope& scope);

    optional<Value> Module::parseFunctionCall(string& name, Scope& scope);

    Function* prototypeFunction(TokenPositon start);

    void prototypeStruct(TokenPositon start);

    bool implementStruct(TokenPositon start);

    void implementFunction(TokenPositon start, Function& func);

    bool implementScope(TokenPositon start, Scope& scope, Function& func);

    bool implementScopeHelper(TokenPositon start, Scope& scope, Function& func);

    bool looksLikeType();

    string dirName();

    bool looksLikeFunction();

    bool looksLikeStruct();
};