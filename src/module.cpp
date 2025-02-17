#include "module.h"
#include "span.h"

Module::Module(const string& dir) {
    this->dir = dir;
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    dataLayout = LLVMGetModuleDataLayout(llvmModule);
    if (dir != "base") {
        moduleDeps.push_back(baseModule);
    }
}

Module::~Module() {
}

void Module::loadTokens() {
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        files.push_back(path.string());
        textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
    }
    tokens.addFileTokens(textByFileByLine);
}

void Module::findStarts() {
    while (true) {
        TokenPositon s = tokens.pos;
        if (looksLikeFunction()) {
            functionStarts.push_back(s);
        } else if (tokens.getToken().type == tt_endl) {
            tokens.nextToken();
            continue;
        } else if (tokens.getToken().type == tt_eof) {
            tokens.nextToken();
            continue;
        } else if (tokens.getToken().type == tt_eot) {
            return;
        } else {
            logError("Don't know what this top level line is", nullptr, true);
            while (true) {
                if (tokens.getToken().type == tt_endl) break;
                if (tokens.getToken().type == tt_eof) break;
                if (tokens.getToken().type == tt_lcur) {
                    Token curStart = tokens.getToken();
                    int curStack = 1;
                    while (curStack != 0) {
                        tokens.nextToken();
                        if (tokens.getToken().type == tt_eot) return;
                        if (tokens.getToken().type == tt_eof) break;
                        if (tokens.getToken().type == tt_rcur) curStack--;
                        if (tokens.getToken().type == tt_lcur) curStack++;
                    }
                    if (tokens.getToken().type == tt_eof) {
                        logError("No closing bracket", &curStart);
                        break;
                    }
                    tokens.nextToken();
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_eof) break;
                    if (tokens.getToken().type == tt_eot) return;
                    logError("Right brackets should be on there own line");
                }
                tokens.nextToken();
            }
        }
    }
}

void Module::setupTypesAndFunctions() {
    vector<Function*> functionDefIsGood(functionStarts.size());
    for (int i = 0; i < functionStarts.size(); i++) {
        functionDefIsGood[i] = prototypeFunction(functionStarts[i]);
    }
    for (int i = 0; i < functionStarts.size(); i++) {
        if (functionDefIsGood[i] != nullptr) {
            implementFunction(functionStarts[i], functionDefIsGood[i]);
        }
    }
}


void Module::logError(const string& err, Token* token, bool wholeLine) {
    if (token == nullptr) token = &tokens.getToken();
    hadError = true;
    std::cout << "\033[31m";
    cout << "Error: " << err << endl;
    std::cout << "\033[0m";
    cout << removeSpaces(textByFileByLine[token->file][token->line]) << endl;
    std::cout << "\033[31m";
    if (wholeLine) {
        for (int i = 0; i <= textByFileByLine[token->file][token->line].size(); i++) {
            cout << "^";
        }
    } else {
        bool startSpaces = true;
        for (int i = 0; i < token->schar; i++) {
            if (isspace(textByFileByLine[token->file][token->line][i]) && startSpaces) {
                continue;
            }
            cout << " ";
            startSpaces = false;
        }
        for (int i = token->schar; i <= token->echar; i++) {
            cout << "^";
        }
    }
    std::cout << "\033[0m";
    cout << endl;
    cout << "Line: " << token->line << " | File: " << files[token->file] << endl;
    cout << "-------------------------------------" << endl;
}

void Module::printResult() {
    if (hadError) {
        std::cout << "\033[31m";
        cout << "Failed" << endl;
        std::cout << "\033[0m";
    } else {
        std::cout << "\033[32m";
        cout << "Success!" << endl;
        std::cout << "\033[0m";
    }
}

optional<Type> Module::typeFromTokens() {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_id) {
        logError("Expected type");
        return nullopt;
    }
    string name = *tokens.getToken().data.str;
    auto t = nameToType.find(name);
    if (t == nameToType.end()) {
        logError("Type doesn't exist");
        return nullopt;
    }
    if (t->second.size() != 1) {
        logError("Ambiguous type");
    }
    Type baseType = t->second[0];
    tokens.nextToken();
    //TODO: type mod

    return baseType;
}

Function* Module::prototypeFunction(TokenPositon start) {
    tokens.pos = start;
    optional<Type> type = typeFromTokens();
    if (!type.has_value()) return nullptr;

    assert(tokens.getToken().type == tt_id);
    string funcName = *tokens.getToken().data.str;
    tokens.nextToken();

    assert(tokens.getToken().type == tt_lpar);
    tokens.nextToken();

    vector<Type> paramTypes;
    vector<string> paramNames;
    if (tokens.getToken().type != tt_rpar) {
        while (true) {
            optional<Type> paramType = typeFromTokens();
            if (!type.has_value()) return nullptr;

            if (tokens.getToken().type != tt_id) {
                logError("Expected parameter name");
                return nullptr;
            }
            string paramName = *tokens.getToken().data.str;
            tokens.nextToken();

            paramTypes.push_back(paramType.value());
            paramNames.push_back(paramName);

            if (tokens.getToken().type == tt_rpar) break;
            if (tokens.getToken().type == tt_com) {
                tokens.nextToken();
                continue;
            }
            logError("Expected , or )");
            return nullptr;
        }
    }
    tokens.nextToken();

    assert(tokens.getToken().type == tt_lcur);

    nameToFunction[funcName].push_back(Function(type.value(), funcName, paramTypes, paramNames, this));
    return &nameToFunction[funcName].back();
}

void Module::implementFunction(TokenPositon start, Function* func) {
    tokens.pos = start;

    while (true) {
        if (tokens.getToken().type == tt_lcur) break;
        tokens.nextToken();
    }
    tokens.nextToken();

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func->llvmValue, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    for (int j = 0; j < func->paramNames.size(); j++) {
        Variable var(func->paramNames[j], func->paramTypes[j], this);
        var.store(func->getParamValue(j));
        func->scope.nameToVariable[var.name] = var;
    }
}


bool Module::looksLikeType() {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_id) {
        tokens.pos = start;
        return false;
    }
    tokens.nextToken();
    // TODO: Type mod

    return true;
}

bool Module::looksLikeFunction() {
    TokenPositon start = tokens.pos;
    if (!looksLikeType()) return false;
    if (tokens.getToken().type != tt_id) return false;
    tokens.nextToken();
    if (tokens.getToken().type != tt_lpar) {
        tokens.pos = start;
        return false;
    }
    while (true) {
        tokens.nextToken();
        if (tokens.getToken().type == tt_rpar) break;
    }
    tokens.nextToken();
    if (tokens.getToken().type != tt_lcur) {
        tokens.pos = start;
        return false;
    }
    int curStack = 1;
    Token curStart = tokens.getToken();
    while (curStack != 0) {
        tokens.nextToken();
        if (tokens.getToken().type == tt_eot) {
            tokens.pos = start;
            return false;
        }
        if (tokens.getToken().type == tt_eof) {
            tokens.pos = start;
            return false;
        }
        if (tokens.getToken().type == tt_rcur) curStack--;
        if (tokens.getToken().type == tt_lcur) curStack++;
    }
    tokens.nextToken();
    if (tokens.getToken().type == tt_endl) return true;
    if (tokens.getToken().type == tt_eof) return true;

    tokens.pos = start;
    return false;
}
