#pragma once
#include "utils.h"
#include "token.h"
#include "function.h"
#include "scope.h"

class Function;

struct typeStartAndTypetype {
    TokenPositon pos;
    bool isStruct;
};

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
    vector<TokenPositon> enumStarts;
    vector<TokenPositon> structStarts;
    vector<Module*> moduleDeps;
    unordered_map<string, typeStartAndTypetype> nameToTypeStart;
    unordered_map<string, bool> nameToTypeDone;
    Tokens tokens;
    bool hadError = false;
    bool hasMain = false;

private:
    optional<Type> typeFromTokens(bool logErrors = true, bool stopAtComma = false, bool stopAtOr = false);

    optional<Value> parseStatment(const vector<TokenType>& del, Scope& scope, int prio = INT_MIN);

    optional<Value> parseValue(Scope& scope);

    optional<Value> Module::parseFunctionCall(string& name, Scope& scope);

    Function* prototypeFunction(TokenPositon start);

    void prototypeStruct(TokenPositon start);

    void prototypeEnum(TokenPositon start);

    bool implementType(typeStartAndTypetype start, bool secondPass = false);

    bool implementStruct(TokenPositon start, bool secondPass = false);

    bool implementEnum(TokenPositon start, bool secondPass = false);

    bool typeFromTokensIsPtr();

    void implementFunction(TokenPositon start, Function& func);

    bool implementScope(TokenPositon start, Scope& scope, Function& func);

    bool implementScopeHelper(TokenPositon start, Scope& scope, Function& func);

    bool looksLikeType();

    string dirName();

    bool looksLikeFunction();

    bool looksLikeEnum();

    bool looksLikeStruct();
};