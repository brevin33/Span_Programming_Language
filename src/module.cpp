#include "module.h"
#include "span.h"

Module::Module(const string& dir) {
    this->dir = dir;
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    if (dir != "base") {
        moduleDeps.push_back(baseModule);
    }
}

Module::~Module() {
    LLVMDisposeModule(llvmModule);
}

void Module::loadTokens() {
    activeModule = this;
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
        } else if (looksLikeEnum()) {
            enumStarts.push_back(s);
        } else if (looksLikeStruct()) {
            structStarts.push_back(s);
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
    // Types
    for (int i = 0; i < structStarts.size(); i++) {
        prototypeStruct(structStarts[i]);
    }
    for (int i = 0; i < enumStarts.size(); i++) {
        prototypeEnum(enumStarts[i]);
    }
    // TODO: prototype enums and such
    for (int i = 0; i < structStarts.size(); i++) {
        implementStruct(structStarts[i]);
    }
    for (int i = 0; i < structStarts.size(); i++) {
        implementStruct(structStarts[i], true);
    }
    for (int i = 0; i < enumStarts.size(); i++) {
        implementEnum(enumStarts[i]);
    }
    for (int i = 0; i < enumStarts.size(); i++) {
        implementEnum(enumStarts[i]);
    }
    // TODO: implement enums and such

    // functions
    vector<Function*> functionDefIsGood(functionStarts.size());
    for (int i = 0; i < functionStarts.size(); i++) {
        functionDefIsGood[i] = prototypeFunction(functionStarts[i]);
    }
    for (int i = 0; i < functionStarts.size(); i++) {
        if (functionDefIsGood[i] != nullptr) {
            implementFunction(functionStarts[i], *functionDefIsGood[i]);
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

bool Module::printResult() {
    char* errorMessage = NULL;
    LLVMBool result = LLVMVerifyModule(baseModule->llvmModule, LLVMReturnStatusAction, &errorMessage);
    bool res;

    cout << "Module " + dir << endl;

    if (result) {
        std::cout << "\033[31m";
        cout << "LLVM Error: ";
        std::cout << "\033[0m";
        std::cout << errorMessage << endl;
        LLVMDisposeMessage(errorMessage);
        hadError = true;
    } else if (hadError) {
        std::cout << "\033[31m";
        cout << "Failed" << endl;
        std::cout << "\033[0m";
    } else {
        std::cout << "\033[32m";
        cout << "Success!" << endl;
        std::cout << "\033[0m";
    }
    cout << "-------------------------------------" << endl;
    return hadError;
}


void Module::compileToObjFile(const string& buildDir) {
    if (hadError) return;
    if (!fs::exists(buildDir)) fs::create_directories(buildDir);
    // Initialize LLVM targets
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();

    // Set target triple
    char* triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(llvmModule, triple);
    printf("Target triple: %s\n", triple);

    // Create target machine
    LLVMTargetRef target;
    char* error = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Error getting target: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    }

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(target, triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

    if (!tm) {
        fprintf(stderr, "Error creating target machine\n");
        return;
    }

    // Set data layout (required for object file emission)
    LLVMTargetDataRef targetData = LLVMCreateTargetDataLayout(tm);
    const char* dataLayout = LLVMGetDataLayoutStr(llvmModule);
    LLVMSetDataLayout(llvmModule, dataLayout);
    LLVMDisposeTargetData(targetData);

    // Verify module before emitting
    char* errorMsg = NULL;
    if (LLVMVerifyModule(llvmModule, LLVMReturnStatusAction, &errorMsg)) {
        fprintf(stderr, "Module verification failed: %s\n", errorMsg);
        LLVMDisposeMessage(errorMsg);
        return;
    }

    // Ensure the output directory exists
    string objFile = buildDir + "/" + dirName() + ".o";
    if (dir == "base") objFile = buildDir + '/' + dir + ".o";
    printf("Output object file: %s\n", objFile.c_str());

    // Emit object file
    if (LLVMTargetMachineEmitToFile(tm, llvmModule, objFile.c_str(), LLVMObjectFile, &error)) {
        fprintf(stderr, "Error writing object file: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    }

    printf("Object file written successfully: %s\n", objFile.c_str());

    // Cleanup
    LLVMDisposeTargetMachine(tm);
    LLVMDisposeMessage(triple);
    cout << "-------------------------------------" << endl;
}

bool Module::typeFromTokensIsPtr() {
    TokenPositon start = tokens.pos;
    while (true) {
        switch (tokens.getToken().type) {
            case tt_mul:
            case tt_and: {
                tokens.nextToken();
                return true;
            }
            case tt_lbar: {
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    tokens.pos = start;
                    return false;
                }
                tokens.nextToken();
                if (tokens.getToken().type != tt_rbar) {
                    tokens.pos = start;
                    return false;
                }
                tokens.nextToken();
                break;
            }
            default: {
                tokens.pos = start;
                return false;
            }
        }
    }
}

optional<Type> Module::typeFromTokens(bool logErrors, bool stopAtComma, bool stopAtOr) {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_id) {
        if (logErrors) logError("Expected type");
        return nullopt;
    }
    string name = *tokens.getToken().data.str;
    tokens.nextToken();
    auto t = nameToType.find(name);
    Type type;
    if (t == nameToType.end()) {
        auto s = nameToTypeStart.find(name);
        if (s == nameToTypeStart.end()) {
            if (logErrors) logError("Type doesn't exist");
            tokens.pos = start;
            return nullopt;
        } else {
            if (!typeFromTokensIsPtr()) {
                if (implementType(s->second)) {
                    type = nameToType.find(name)->second.front();
                } else {
                    tokens.pos = start;
                    return nullopt;
                }
            } else {
                type = Type(LLVMPointerType(LLVMInt1Type(), 0), name, this);
            }
        }
    } else {
        if (t->second.size() != 1) {
            tokens.pos = start;
            if (logErrors) logError("Ambiguous type");
        }
        type = t->second[0];
    }
    while (true) {
        switch (tokens.getToken().type) {
            case tt_mul: {
                type = type.ptr();
                tokens.nextToken();
                break;
            }
            case tt_bitand: {
                type = type.ref();
                tokens.nextToken();
                break;
            }
            case tt_lbar: {
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    if (logErrors) logError("Expected number");
                    tokens.pos = start;
                    return nullopt;
                }
                u64 number = tokens.getToken().data.uint;
                tokens.nextToken();
                if (tokens.getToken().type != tt_rbar) {
                    if (logErrors) logError("Expected ]");
                    tokens.pos = start;
                    return nullopt;
                }
                tokens.nextToken();
                type = type.vec(number);
                break;
            }
            case tt_bitor: {
                if (stopAtOr) return type;
                tokens.nextToken();
                vector<Type> impleStructTypes;
                impleStructTypes.push_back(type);
                while (true) {
                    optional<Type> nextType = typeFromTokens(logErrors, false, true);
                    if (!nextType.has_value()) {
                        tokens.pos = start;
                        return nullopt;
                    }
                    impleStructTypes.push_back(nextType.value());
                    if (tokens.getToken().type != tt_bitor) break;
                    tokens.nextToken();
                }
                string typeName = "(";
                vector<string> structElmName;
                vector<int> enumValues;
                for (int i = 0; i < impleStructTypes.size(); i++) {
                    typeName += impleStructTypes[i].name + "|";
                    structElmName.push_back(impleStructTypes[i].name);
                    enumValues.push_back(i);
                }
                typeName += ")";
                return Type(typeName, impleStructTypes, structElmName, enumValues, this, true);
            }
            case tt_com: {
                if (stopAtComma) return type;
                tokens.nextToken();
                vector<Type> impleStructTypes;
                impleStructTypes.push_back(type);
                while (true) {
                    optional<Type> nextType = typeFromTokens(logErrors, true, stopAtOr);
                    if (!nextType.has_value()) {
                        tokens.pos = start;
                        return nullopt;
                    }
                    impleStructTypes.push_back(nextType.value());
                    if (tokens.getToken().type != tt_com) break;
                    if (tokens.getToken().type != tt_bitor) break;
                    tokens.nextToken();
                }
                string typeName = "(";
                vector<string> structElmName;
                for (int i = 0; i < impleStructTypes.size(); i++) {
                    typeName += impleStructTypes[i].name;
                    structElmName.push_back(to_string(i));
                }
                typeName += ")";
                type = Type(typeName, impleStructTypes, structElmName, this);
            }
            default: {
                return type;
            }
        }
    }
}

optional<Value> Module::parseStatment(const vector<TokenType>& del, Scope& scope, int prio) {
    TokenPositon start = tokens.pos;
    optional<Value> lval = parseValue(scope);
    if (!lval.has_value()) return nullopt;
    while (true) {
        for (int i = 0; i < del.size(); i++) {
            if (tokens.getToken().type == del[i]) return lval;
        }
        switch (tokens.getToken().type) {
            case tt_com: {
                vector<Value> vals;
                vals.push_back(lval.value());
                tokens.nextToken();
                vector<TokenType> d = del;
                d.push_back(tt_com);
                while (true) {
                    optional<Value> val = parseStatment(d, scope);
                    if (!val.has_value()) {
                        tokens.pos = start;
                        return nullopt;
                    }
                    vals.push_back(val.value());
                    if (tokens.getToken().type == tt_com) {
                        tokens.nextToken();
                        continue;
                    }
                    bool donwWithThis = false;
                    for (int i = 0; i < del.size(); i++) {
                        if (tokens.getToken().type == del[i]) {
                            donwWithThis = true;
                            break;
                        }
                    }
                    if (donwWithThis) break;
                    logError("Expected a end line or a ,");
                    tokens.pos = start;
                    return nullopt;
                }
                string typeName = "(";
                vector<Type> types;
                vector<string> structElmName;
                vector<LLVMValueRef> valrefs;
                for (int i = 0; i < vals.size(); i++) {
                    typeName += vals[i].type.name;
                    types.push_back(vals[i].type);
                    structElmName.push_back(to_string(i));
                    valrefs.push_back(vals[i].llvmValue);
                }
                typeName += ")";
                Type t(typeName, types, structElmName, this);
                LLVMValueRef myStruct = LLVMGetUndef(t.llvmType);
                for (int i = 0; i < vals.size(); i++) {
                    myStruct = LLVMBuildInsertValue(builder, myStruct, valrefs[i], i, "inserted");
                }
                Value v(myStruct, t, this, true);
                return v;
            }
            case tt_lbar: {
                tokens.nextToken();
                optional<Value> rval = parseStatment({ tt_rbar }, scope);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = index(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't add types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                tokens.nextToken();
                break;
            }
            case tt_add: {
                if (prio >= 3) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 3);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = add(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't add types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_and: {
                if (prio >= -1) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, -1);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = and(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't and types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_or: {
                if (prio >= -1) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, -1);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = or (lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't or types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_mul: {
                if (prio >= 4) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 4);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = mul(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't multiple types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_dot: {
                tokens.nextToken();
                if (tokens.getToken().type != tt_id && tokens.getToken().type != tt_int) {
                    logError("Expected method or member");
                    tokens.pos = start;
                    return nullopt;
                }
                string str;
                if (tokens.getToken().type == tt_id) str = *tokens.getToken().data.str;
                else
                    str = to_string(tokens.getToken().data.uint);
                tokens.nextToken();
                if (tokens.getToken().type == tt_lpar) {
                    // method
                    str = lval.value().type.actualType().name + "." + str;
                    optional<Value> val = parseFunctionCall(str, scope, &lval.value());
                    if (!val.has_value()) {
                        tokens.pos = start;
                        return nullopt;
                    }
                    lval = val;
                } else {
                    bool found = false;
                    for (int i = 0; i < lval.value().type.elemNames.size(); i++) {
                        if (lval.value().type.elemNames[i] != str) {
                            continue;
                        }
                        if (lval.value().type.isEnum()) lval = lval.value().enumVal(i);
                        else
                            lval = lval.value().structVal(i);
                        found = true;
                        break;
                    }
                    if (!found) {
                        tokens.lastToken();
                        logError("Didn't find a member with this name");
                        tokens.pos = start;
                        return nullopt;
                    }
                }
                break;
            }
            case tt_div: {
                if (prio >= 4) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 4);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = div(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't divide types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_sub: {
                if (prio >= 3) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 3);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = sub(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't subtract types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_eqeq: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = equal(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check equals types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_greq: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = greaterThanOrEqual(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check greater than or equals types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_leeq: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = lessThanOrEqual(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check less than or equals types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_le: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = lessThan(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check less than types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_gr: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = greaterThan(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check greater than types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_neq: {
                if (prio >= 0) return lval;
                tokens.nextToken();
                optional<Value> rval = parseStatment(del, scope, 0);
                if (!rval.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = notEqual(lval.value(), rval.value());
                if (!addVal.has_value()) {
                    logError("Can't check not equals types of " + lval.value().type.name + " with " + rval.value().type.name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_as: {
                if (prio >= 1) return lval;
                tokens.nextToken();
                optional<Type> rtype = typeFromTokens();
                if (!rtype.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = as(lval.value(), rtype.value());
                if (!addVal.has_value()) {
                    logError("Can't bit cast types of " + lval.value().type.name + " to " + rtype.value().name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }
            case tt_to: {
                if (prio >= 1) return lval;
                tokens.nextToken();
                optional<Type> rtype = typeFromTokens();
                if (!rtype.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                optional<Value> addVal = to(lval.value(), rtype.value());
                if (!addVal.has_value()) {
                    logError("Can't cast types of " + lval.value().type.name + " to " + rtype.value().name);
                    tokens.pos = start;
                    return nullopt;
                }
                lval = addVal;
                break;
            }

            default: {
                logError("Expected an operation to apply");
                tokens.pos = start;
                return nullopt;
            }
        }
    }
    return nullopt;
}

optional<Value> Module::parseValue(Scope& scope) {
    TokenPositon start = tokens.pos;
    switch (tokens.getToken().type) {
        case tt_id: {
            Token s = tokens.getToken();
            optional<Type> type = typeFromTokens(false);
            // Type operation
            if (type.has_value()) {
                if (tokens.getToken().type != tt_dot) {
                    logError("Expected a dot . after a Type to use it");
                    tokens.pos = start;
                    return nullopt;
                }
                tokens.nextToken();
                if (tokens.getToken().type != tt_id) {
                    logError("Expected a idenfier");
                    tokens.pos = start;
                    return nullopt;
                }
                string str = *tokens.getToken().data.str;
                tokens.nextToken();
                if (type.value().isEnum()) {
                    TokenPositon enumStart = tokens.pos;
                    // enum ops
                    for (int i = 0; i < type.value().elemNames.size(); i++) {
                        if (type.value().elemNames[i] == str) {
                            if (type.value().elemTypes[i].name == "void") {
                                LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
                                Scope* s = &scope;
                                while (s->parent != nullptr) {
                                    s = s->parent;
                                }
                                LLVMPositionBuilderAtEnd(builder, s->blocks.front());
                                LLVMValueRef enumAlloca = LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), str.size() + 1), "enum");
                                LLVMPositionBuilderAtEnd(builder, curBlock);

                                Value e = createEnum(type.value(), nullptr, i);
                                return e;
                            } else {
                                if (tokens.getToken().type != tt_lpar) {
                                    logError("Expected a )");
                                    tokens.pos = start;
                                    return nullopt;
                                }
                                tokens.nextToken();
                                optional<Value> val = parseStatment({ tt_rpar }, scope);
                                if (!val.has_value()) {
                                    tokens.pos = start;
                                    return nullopt;
                                }
                                val = val.value().implCast(type.value().elemTypes[i]);
                                if (!val.has_value()) {
                                    logError("given to enum doesn't match enum");
                                    tokens.pos = start;
                                    return nullopt;
                                }
                                Value e = createEnum(type.value(), &val.value(), i);
                                tokens.nextToken();
                                return e;
                            }
                        }
                    }
                    tokens.pos = enumStart;
                }


                tokens.lastToken();
                logError("didn't find a valid operation on type " + type.value().name + " called " + str);
                tokens.pos = start;
                return nullopt;
            }

            string name = *tokens.getToken().data.str;
            tokens.nextToken();
            // func call
            if (tokens.getToken().type == tt_lpar) {
                optional<Value> val = parseFunctionCall(name, scope);
                if (!val.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                return val.value();
            }
            // get var
            optional<Variable*> var = scope.getVariableFromName(name);
            if (!var.has_value()) {
                logError("No variable with this name", &s);
                tokens.pos = start;
                return nullopt;
            }
            return var.value()->value;
        }
        case tt_lcur: {
            vector<Value> vals;
            tokens.nextToken();
            while (true) {
                optional<Value> val = parseStatment({ tt_com, tt_rcur }, scope);
                if (!val.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                vals.push_back(val.value());
                if (tokens.getToken().type == tt_com) {
                    tokens.nextToken();
                    continue;
                }
                if (tokens.getToken().type == tt_rcur) break;
                logError("Expected a closing } or a ,");
                tokens.pos = start;
                return nullopt;
            }
            tokens.nextToken();
            string typeName = "(";
            vector<Type> types;
            vector<string> structElmName;
            vector<LLVMValueRef> valrefs;
            for (int i = 0; i < vals.size(); i++) {
                typeName += vals[i].type.name;
                types.push_back(vals[i].type);
                structElmName.push_back(to_string(i));
                valrefs.push_back(vals[i].llvmValue);
            }
            typeName += ")";
            Type t(typeName, types, structElmName, this);
            LLVMValueRef myStruct = LLVMGetUndef(t.llvmType);
            for (int i = 0; i < vals.size(); i++) {
                myStruct = LLVMBuildInsertValue(builder, myStruct, valrefs[i], i, "inserted");
            }
            Value v(myStruct, t, this, true);
            return v;
        }
        case tt_lpar: {
            tokens.nextToken();
            optional<Value> v = parseStatment({ tt_rpar, tt_endl }, scope);
            if (!v.has_value()) {
                tokens.pos = start;
                return nullopt;
            }
            if (tokens.getToken().type == tt_endl) {
                tokens.pos = start;
                logError("Never found closing )");
                return nullopt;
            }
            tokens.nextToken();
            return v.value();
        }
        case tt_int: {
            u64 num = tokens.getToken().data.uint;
            tokens.nextToken();
            return Value(num, this);
        }
        case tt_float: {
            f64 num = tokens.getToken().data.dec;
            tokens.nextToken();
            return Value(num, this);
        }
        case tt_str: {
            string str = *tokens.getToken().data.str;

            LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
            Scope* s = &scope;
            while (s->parent != nullptr) {
                s = s->parent;
            }
            LLVMPositionBuilderAtEnd(builder, s->blocks.front());
            LLVMValueRef strAlloca = LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), str.size() + 1), "str");
            LLVMPositionBuilderAtEnd(builder, curBlock);

            for (size_t i = 0; i < str.size() + 1; i++) {
                LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, false);
                LLVMValueRef charPtr = LLVMBuildGEP2(builder, LLVMInt8Type(), strAlloca, &index, 1, "charPtr");
                LLVMBuildStore(builder, LLVMConstInt(LLVMInt8Type(), str[i], false), charPtr);
            }
            Type charptr = Type("char*", baseModule);
            tokens.nextToken();

            Token t = tokens.getToken();

            return Value(strAlloca, charptr, this, true);
        }
        case tt_sub: {
            tokens.nextToken();
            if (tokens.getToken().type == tt_sub) {
                logError("Cann't have two - in a row");
                tokens.pos = start;
                return nullopt;
            }
            optional<Value> val = parseValue(scope);
            if (!val.has_value()) {
                tokens.pos = start;
                return nullopt;
            }
            optional<Value> negVal = val.value().negate();
            return negVal;
        }
        default: {
            logError("Expected a value");
            return nullopt;
        }
    }
    return nullopt;
}

optional<Value> Module::parseFunctionCall(string& name, Scope& scope, Value* caller) {

    tokens.lastToken();
    Token funcNameToken = tokens.getToken();
    tokens.nextToken();

    TokenPositon start = tokens.pos;
    vector<Value> vals;
    if (caller != nullptr) {
        vals.push_back(*caller);
    }
    assert(tokens.getToken().type == tt_lpar);
    tokens.nextToken();
    if (tokens.getToken().type != tt_rpar) {
        while (true) {
            optional<Value> val = parseStatment({ tt_com, tt_rpar }, scope);
            if (!val.has_value()) {
                tokens.pos = start;
                return nullopt;
            }
            vals.push_back(val.value());
            if (tokens.getToken().type == tt_com) {
                tokens.nextToken();
                continue;
            }
            if (tokens.getToken().type == tt_rpar) break;
            logError("Expected a closing ) or a ,");
            tokens.pos = start;
            return nullopt;
        }
    }

    tokens.nextToken();

    auto t = nameToFunction.find(name);
    if (t == nameToFunction.end()) {
        logError("No function with name of " + name, &funcNameToken);
        return {};
    }
    vector<Function>& funcs = t->second;
    Function* funcToCall = nullptr;
    for (int j = 0; j < funcs.size(); j++) {
        if (funcs[j].paramNames.size() == vals.size() || (funcs[j].variadic && funcs[j].paramNames.size() <= vals.size())) {
            bool match = true;
            for (int k = 0; k < funcs[j].paramNames.size(); k++) {
                if (funcs[j].paramTypes[k] != vals[k].type && funcs[j].paramTypes[k] != vals[k].type.actualType()) match = false;
            }
            if (match) {
                funcToCall = &funcs[j];
                break;
            }
        }
    }

    if (funcToCall == nullptr) {
        // see if there is an way to cast into a function
        for (int j = 0; j < funcs.size(); j++) {
            if (funcs[j].paramNames.size() == vals.size() || (funcs[j].variadic && funcs[j].paramNames.size() <= vals.size())) {
                bool match = true;
                for (int k = 0; k < funcs[j].paramNames.size(); k++) {
                    bool useable = funcs[j].paramTypes[k] == vals[k].type;
                    useable = useable || (funcs[j].paramTypes[k].isNumber() && vals[k].type.actualType().isNumber());
                    if (!useable) match = false;
                }
                if (match) {
                    if (funcToCall != nullptr) {
                        logError("Function call is ambiguous", &funcNameToken);
                        tokens.pos = start;
                        return nullopt;
                    }
                    funcToCall = &funcs[j];
                }
            }
        }

        if (funcToCall == nullptr) {
            logError("No function overload uses given arguments", &funcNameToken);
            tokens.pos = start;
            return nullopt;
        }
    }

    for (int k = 0; k < funcToCall->paramNames.size(); k++) {
        if (funcToCall->paramTypes[k] == vals[k].type.actualType()) {
            vals[k] = vals[k].actualValue();
        }
        if (funcToCall->paramTypes[k] != vals[k].type) {
            optional<Value> v = vals[k].cast(funcToCall->paramTypes[k]);
            assert(v.has_value());
            vals[k] = v.value();
        }
    }
    for (int k = funcToCall->paramNames.size(); k < vals.size(); k++) {
        // variadic arg
        optional<Value> v = vals[k].variadicCast();
        if (!v.has_value()) {
            logError("can't pass values lager than 64 bits via variadic arguments");
            return nullopt;
        }
        vals[k] = v.value();
    }

    return funcToCall->call(vals, this);
}

Function* Module::prototypeFunction(TokenPositon start) {
    tokens.pos = start;
    optional<Type> type = typeFromTokens();
    if (!type.has_value()) return nullptr;

    optional<Type> methodType = typeFromTokens(false);
    if (methodType.has_value()) {
        if (tokens.getToken().type != tt_dot) {
            logError("Expected a dot . after a Type to make a method");
            return nullptr;
        } else {
            tokens.nextToken();
        }
    }
    assert(tokens.getToken().type == tt_id);
    string funcName = *tokens.getToken().data.str;
    if (methodType.has_value()) funcName = methodType.value().name + "." + funcName;
    tokens.nextToken();

    assert(tokens.getToken().type == tt_lpar);
    tokens.nextToken();

    vector<Type> paramTypes;
    vector<string> paramNames;
    if (methodType.has_value()) {
        paramTypes.push_back(methodType.value());
        paramNames.push_back("this");
    }
    bool variadicArgs = false;
    if (tokens.getToken().type != tt_rpar) {
        while (true) {

            if (tokens.getToken().type == tt_elips) {
                tokens.nextToken();
                if (tokens.getToken().type != tt_rpar) {
                    logError("Expected a )");
                    return nullptr;
                }
                variadicArgs = true;
                break;
            }

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

    bool external = true;
    if (tokens.getToken().type == tt_lcur) external = false;

    if (funcName == "main") {
        if (type.value().name != "void" && type.value().name != "i32") {
            logError("return type of main must be int, i32 or void", nullptr, true);
            return nullptr;
        }
        if (hasMain) {
            logError("already has a main function. can't have multiple main", nullptr, true);
            return nullptr;
        }
        hasMain = true;
        nameToFunction[funcName].push_back(Function(nameToType["i32"].front(), funcName, paramTypes, paramNames, this, variadicArgs, external, methodType));
    } else {
        nameToFunction[funcName].push_back(Function(type.value(), funcName, paramTypes, paramNames, this, variadicArgs, external, methodType));
    }
    return &nameToFunction[funcName].back();
}

void Module::prototypeEnum(TokenPositon start) {
    tokens.pos = start;
    tokens.nextToken();
    string name = *tokens.getToken().data.str;
    nameToTypeStart[name] = { start, false };
    return;
}

bool Module::implementType(typeStartAndTypetype start, bool secondPass) {
    if (start.isStruct) {
        return implementStruct(start.pos, secondPass);
    } else {
        return implementEnum(start.pos, secondPass);
    }
    return false;
}

void Module::prototypeStruct(TokenPositon start) {
    tokens.pos = start;
    tokens.nextToken();
    string name = *tokens.getToken().data.str;
    nameToTypeStart[name] = { start, true };
    return;
}

bool Module::implementEnum(TokenPositon start, bool secondPass) {
    tokens.pos = start;
    assert(tokens.getToken().type == tt_enum);
    tokens.nextToken();
    string name = *tokens.getToken().data.str;
    if (secondPass && nameToTypeDone[name] == false) return false;
    if (!secondPass) {
        if (nameToTypeDone.find(name) != nameToTypeDone.end()) {
            if (nameToTypeDone[name] == false) {
                logError("Circular Dependency");
                tokens.pos = start;
                return false;
            } else {
                return true;
            }
        }
        nameToTypeDone[name] = false;
    }
    tokens.nextToken();
    tokens.nextToken();
    if (tokens.getToken().type != tt_endl) {
        logError("Expected end line after {");
        return false;
    }
    tokens.nextToken();

    vector<Type> enumTypes;
    vector<string> enumElementNames;
    vector<int> enumElementValues;
    int valueCounter = 0;
    while (true) {
        if (tokens.getToken().type == tt_rcur) break;
        if (tokens.getToken().type != tt_id) {
            logError("expected name of enum value");
            return false;
        }
        string elName = *tokens.getToken().data.str;
        tokens.nextToken();
        if (tokens.getToken().type == tt_endl) {
            tokens.nextToken();
            enumTypes.push_back(nameToType["void"].front());
            enumElementNames.push_back(elName);
            enumElementValues.push_back(valueCounter++);
        } else if (tokens.getToken().type == tt_lpar) {
            tokens.nextToken();
            optional<Type> t = typeFromTokens();
            if (!t.has_value()) {
                return false;
            }
            if (tokens.getToken().type != tt_rpar) {
                logError("expected closing )");
                return false;
            }
            tokens.nextToken();
            if (tokens.getToken().type != tt_endl) {
                logError("expected a newline after the closing ) when making an enum");
                return false;
            }
            tokens.nextToken();
            enumTypes.push_back(t.value());
            enumElementNames.push_back(elName);
            enumElementValues.push_back(valueCounter++);
        } else {
            logError("expected a new line or a (type) newline");
            return false;
        }
    }

    Type Enum(name, enumTypes, enumElementNames, enumElementValues, this);
    nameToTypeDone[name] = true;
    return true;
}

bool Module::implementStruct(TokenPositon start, bool secondPass) {
    tokens.pos = start;
    assert(tokens.getToken().type == tt_struct);
    tokens.nextToken();
    string name = *tokens.getToken().data.str;
    if (secondPass && nameToTypeDone[name] == false) return false;
    if (!secondPass) {
        if (nameToTypeDone.find(name) != nameToTypeDone.end()) {
            if (nameToTypeDone[name] == false) {
                logError("Circular Dependency");
                tokens.pos = start;
                return false;
            } else {
                return true;
            }
        }
        nameToTypeDone[name] = false;
    }
    tokens.nextToken();
    tokens.nextToken();
    if (tokens.getToken().type != tt_endl) {
        logError("Expected end line after {");
        return false;
    }
    tokens.nextToken();

    vector<Type> structTypes;
    vector<string> structElementNames;
    while (true) {
        if (tokens.getToken().type == tt_rcur) break;
        optional<Type> t = typeFromTokens();
        if (!t.has_value()) {
            return false;
        }
        if (tokens.getToken().type != tt_id) {
            logError("expected name of struct value");
            return false;
        }
        string elName = *tokens.getToken().data.str;
        tokens.nextToken();
        if (tokens.getToken().type != tt_endl) {
            logError("expected endline after defining struct element");
            return false;
        }
        tokens.nextToken();
        structTypes.push_back(t.value());
        structElementNames.push_back(elName);
    }

    Type Struct(name, structTypes, structElementNames, this);
    nameToTypeDone[name] = true;
    return true;
}



#define implementScopeRecoverError                                                                                                                                                                                                   \
    while (true) {                                                                                                                                                                                                                   \
        if (tokens.getToken().type == tt_endl) break;                                                                                                                                                                                \
        if (tokens.getToken().type == tt_rcur) return false;                                                                                                                                                                         \
        tokens.nextToken();                                                                                                                                                                                                          \
    }                                                                                                                                                                                                                                \
    continue;

bool Module::implementScopeHelper(TokenPositon start, Scope& scope, Function& func) {
    tokens.pos = start;
    assert(tokens.getToken().type == tt_lcur);
    tokens.nextToken();

    if (tokens.getToken().type != tt_endl) {
        logError("Nothing else should be on this line");
        while (true) {
            if (tokens.getToken().type == tt_endl) break;
            if (tokens.getToken().type == tt_rcur) return false;
            tokens.nextToken();
        }
    }

    while (true) {
        assert(tokens.getToken().type == tt_endl);
        while (tokens.getToken().type == tt_endl)
            tokens.nextToken();
        bool err = false;
        if (tokens.getToken().type == tt_rcur) {
            break;
        }

        // break
        if (tokens.getToken().type == tt_break) {
            tokens.nextToken();
            int numBreak = 1;
            if (tokens.getToken().type == tt_int) {
                numBreak = tokens.getToken().data.uint;
                tokens.nextToken();
            }
            if (tokens.getToken().type != tt_endl) {
                logError("Expected end line after break");
                implementScopeRecoverError
            }
            Scope* s = &scope;
            while (true) {
                if (s->canBreak) numBreak--;
                if (numBreak == 0) break;
                s = s->parent;
                if (s == nullptr) break;
            }
            if (s == nullptr || s->parent == nullptr) {
                logError("can't break that far", nullptr, true);
                implementScopeRecoverError
            }
            s = s->parent;
            s->gotoLast();
            tokens.nextToken();
            if (tokens.getToken().type != tt_rcur) {
                logError("Must end scope after calling break");
                tokens.lastToken();
                continue;
            }
            return true;
        }


        // continue
        if (tokens.getToken().type == tt_continue) {
            tokens.nextToken();
            int numBreak = 1;
            if (tokens.getToken().type == tt_int) {
                numBreak = tokens.getToken().data.uint;
                tokens.nextToken();
            }
            if (tokens.getToken().type != tt_endl) {
                logError("Expected end line after continue");
                implementScopeRecoverError
            }
            Scope* s = &scope;
            while (true) {
                if (s->canBreak) numBreak--;
                if (numBreak == 0) break;
                s = s->parent;
                if (s == nullptr) break;
            }
            if (s == nullptr) {
                logError("can't continue that far", nullptr, true);
                implementScopeRecoverError
            }
            s->gotoFront();
            tokens.nextToken();
            if (tokens.getToken().type != tt_rcur) {
                logError("Must end scope after calling continue");
                tokens.lastToken();
                continue;
            }
            return true;
        }

        //for
        if (tokens.getToken().type == tt_for) {
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func.llvmValue, "merge");
            scope.addBlock(merge_block);
            tokens.nextToken();
            LLVMBasicBlockRef whileStart = LLVMAppendBasicBlock(func.llvmValue, "whileStart");
            LLVMBasicBlockRef whileCon = LLVMAppendBasicBlock(func.llvmValue, "whileCon");
            LLVMBasicBlockRef whileBody = LLVMAppendBasicBlock(func.llvmValue, "whileBody");
            LLVMBasicBlockRef whileAfter = LLVMAppendBasicBlock(func.llvmValue, "whileAfter");

            Scope whileScope(&scope, whileAfter, true);
            whileScope.addBlock(whileStart);
            whileScope.addBlock(whileCon);
            whileScope.addBlock(whileBody);

            LLVMBuildBr(builder, whileStart);
            LLVMPositionBuilderAtEnd(builder, whileStart);

            TokenPositon before = tokens.pos;
            bool inForLoop = false;
            while (tokens.getToken().type != tt_endl) {
                tokens.nextToken();
                if (tokens.getToken().type == tt_in) inForLoop = true;
            }
            tokens.pos = before;
            if (inForLoop) {
                optional<Type> type = typeFromTokens();
                if (tokens.getToken().type != tt_id) {
                    logError("Expected variable name");
                    implementScopeRecoverError
                }
                string varName = *tokens.getToken().data.str;
                Value val;
                if (type.has_value()) {
                    if (type.value().isInt() || type.value().isUInt()) {
                    } else {
                        logError("must be int or uint");
                        implementScopeRecoverError
                    }
                    LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
                    LLVMPositionBuilderAtEnd(builder, func.entry);
                    bool worked = scope.addVariable(Variable(varName, type.value(), this));
                    LLVMPositionBuilderAtEnd(builder, curBlock);
                    if (!worked) {
                        logError("failed to add varible");
                        worked = false;
                        implementScopeRecoverError
                    }
                    Variable* var = scope.getVariableFromName(varName).value();
                    val = var->value;
                } else {
                    // todo: for pre existing variable
                }
                tokens.nextToken();
                if (tokens.getToken().type != tt_in) {
                    logError("Expected in");
                    implementScopeRecoverError
                }
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    logError("Expected int");
                    implementScopeRecoverError
                }
                u64 start = tokens.getToken().data.uint;
                tokens.nextToken();
                if (tokens.getToken().type != tt_elips && tokens.getToken().type != tt_elipseq) {
                    logError("Expected ...");
                    implementScopeRecoverError
                }
                bool includeEnd = tokens.getToken().type == tt_elipseq;
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    logError("Expected int");
                    implementScopeRecoverError
                }
                u64 end = tokens.getToken().data.uint;
                tokens.nextToken();
                if (tokens.getToken().type != tt_lcur) {
                    logError("Expected {");
                    implementScopeRecoverError
                }
                LLVMPositionBuilderAtEnd(builder, whileStart);
                Variable* var = scope.getVariableFromName(varName).value();
                var->value.type.getBitWidth();
                Value startv = Value(LLVMConstInt(LLVMInt32Type(), start, 0), nameToType["i32"].front(), this, true);
                optional<Value> startAsVal = startv.cast(var->value.type.dereference());
                var->store(startAsVal.value());
                LLVMBuildBr(builder, whileCon);

                LLVMPositionBuilderAtEnd(builder, whileCon);
                val = val.refToVal();
                if (!startAsVal.has_value() || (!startAsVal.value().type.isNumber())) {
                    logError("Expected if statment to have a bool or number as value");
                    implementScopeRecoverError
                }
                LLVMValueRef condition;
                Value endv = Value(LLVMConstInt(LLVMInt32Type(), end, 0), nameToType["i32"].front(), this, true);
                optional<Value> endAsVal = endv.cast(var->value.type.dereference());
                if (startAsVal.value().type.isFloat()) {
                    if (includeEnd) {
                        if (start < end) {
                            condition = LLVMBuildFCmp(builder, LLVMRealOLE, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        } else {
                            condition = LLVMBuildFCmp(builder, LLVMRealOGT, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        }
                    } else {
                        if (start < end) {
                            condition = LLVMBuildFCmp(builder, LLVMRealOLT, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        } else {
                            condition = LLVMBuildFCmp(builder, LLVMRealOGE, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        }
                    }
                } else {
                    if (includeEnd) {
                        if (start < end) {
                            condition = LLVMBuildICmp(builder, LLVMIntSLE, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        } else {
                            condition = LLVMBuildICmp(builder, LLVMIntSGT, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        }
                    } else {
                        if (start < end) {
                            condition = LLVMBuildICmp(builder, LLVMIntSLT, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        } else {
                            condition = LLVMBuildICmp(builder, LLVMIntSGE, endAsVal.value().llvmValue, val.llvmValue, "cmp");
                        }
                    }
                }
                LLVMBuildCondBr(builder, condition, merge_block, whileBody);

                LLVMPositionBuilderAtEnd(builder, whileAfter);
                val = var->value;
                val = val.refToVal();
                i64 inc;
                if (start < end) {
                    inc = 1;
                } else {
                    inc = -1;
                }
                Value one = Value(inc, this);
                optional<Value> valAddOpt = add(val, one);
                if (!valAddOpt.has_value()) {
                    logError("Failed to add values");
                    implementScopeRecoverError
                }
                Value valAdd = valAddOpt.value();
                var->store(valAdd);
                LLVMBuildBr(builder, whileCon);
            } else {

                // really bad close this tab
                // is setting value
                {
                    vector<Value> setVals;
                    bool worked = true;
                    while (true) {
                        optional<Type> type = typeFromTokens(false);
                        if (type.has_value()) {
                            // type
                            if (tokens.getToken().type != tt_id) {
                                logError("Expected variable name");
                                worked = false;
                                break;
                            }
                            string varName = *tokens.getToken().data.str;
                            LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
                            LLVMPositionBuilderAtEnd(builder, func.entry);
                            bool worked = scope.addVariable(Variable(varName, type.value(), this));
                            LLVMPositionBuilderAtEnd(builder, curBlock);
                            if (!worked) {
                                logError("Already have a variable with this name in scope");
                                worked = false;
                                break;
                            }
                            Variable* var = scope.getVariableFromName(varName).value();
                            Value val = var->value;
                            setVals.push_back(val);
                            tokens.nextToken();
                        } else {
                            // statment
                            optional<Value> val = parseStatment({ tt_eq, tt_addeq, tt_subeq, tt_muleq, tt_diveq, tt_endl, tt_com }, scope);
                            if (!val.has_value()) {
                                worked = false;
                                break;
                            }
                            setVals.push_back(val.value());
                        }
                        if (tokens.getToken().type == tt_com) {
                            tokens.nextToken();
                            continue;
                        }
                        break;
                    }
                    if (!worked) {
                        implementScopeRecoverError
                    }
                    if (tokens.getToken().type == tt_endl) continue;
                    for (int i = 0; i < setVals.size(); i++) {
                        Value val = setVals[i];
                        if (!val.type.isRef()) {
                            logError("Can't assign to a value on lhs", nullptr, true);
                            implementScopeRecoverError
                        }
                    }

                    if (setVals.size() == 1) {
                        optional<Value> rval;
                        Value val = setVals[0];
                        if (tokens.getToken().type == tt_addeq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = add(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't add types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_subeq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = sub(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't sub types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_muleq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = mul(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't mul types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_diveq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = div(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't div types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_eq) {
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                        } else {
                            logError("expected an assignment operator");
                            implementScopeRecoverError
                        }
                        optional<Value> valCast = rval.value().implCast(val.type.dereference());
                        if (!valCast.has_value()) {
                            logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rval.value().type.name, nullptr, true);
                            continue;
                        }
                        val.store(valCast.value());
                    } else {
                        optional<Value> rval;
                        if (tokens.getToken().type == tt_eq) {
                            tokens.nextToken();
                            rval = parseStatment({ tt_semi }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                        } else {
                            logError("expected an assignment operator");
                            implementScopeRecoverError
                        }
                        if (!rval.value().type.isStruct()) {
                            logError("multiple assignment only work with a struct on the right hand side");
                            implementScopeRecoverError
                        }
                        if (setVals.size() != rval.value().type.elemNames.size()) {
                            logError("right hand side struct contains " + to_string(rval.value().type.elemNames.size()) + " values when setting " + to_string(setVals.size()) + " values", nullptr, true);
                            implementScopeRecoverError
                        }

                        for (int i = 0; i < setVals.size(); i++) {
                            Value val = setVals[i];
                            Value rsval = rval.value().structVal(i);
                            optional<Value> valCast = rsval.implCast(val.type.dereference());
                            if (!valCast.has_value()) {
                                logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rsval.type.name, nullptr, true);
                                continue;
                            }
                            val.store(valCast.value().actualValue());
                        }
                    }
                }
                tokens.nextToken();

                LLVMBuildBr(builder, whileCon);
                LLVMPositionBuilderAtEnd(builder, whileCon);
                optional<Value> branchVal = parseStatment({ tt_semi }, scope);
                if (!branchVal.has_value()) {
                    implementScopeRecoverError
                }
                tokens.nextToken();

                LLVMPositionBuilderAtEnd(builder, whileAfter);
                {
                    vector<Value> setVals;
                    bool worked = true;
                    while (true) {
                        optional<Type> type = typeFromTokens(false);
                        if (type.has_value()) {
                            // type
                            if (tokens.getToken().type != tt_id) {
                                logError("Expected variable name");
                                worked = false;
                                break;
                            }
                            string varName = *tokens.getToken().data.str;
                            LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
                            LLVMPositionBuilderAtEnd(builder, func.entry);
                            bool worked = scope.addVariable(Variable(varName, type.value(), this));
                            LLVMPositionBuilderAtEnd(builder, curBlock);
                            if (!worked) {
                                logError("Already have a variable with this name in scope");
                                worked = false;
                                break;
                            }
                            Variable* var = scope.getVariableFromName(varName).value();
                            Value val = var->value;
                            setVals.push_back(val);
                            tokens.nextToken();
                        } else {
                            // statment
                            optional<Value> val = parseStatment({ tt_eq, tt_addeq, tt_subeq, tt_muleq, tt_diveq, tt_endl, tt_com }, scope);
                            if (!val.has_value()) {
                                worked = false;
                                break;
                            }
                            setVals.push_back(val.value());
                        }
                        if (tokens.getToken().type == tt_com) {
                            tokens.nextToken();
                            continue;
                        }
                        break;
                    }
                    if (!worked) {
                        implementScopeRecoverError
                    }
                    if (tokens.getToken().type == tt_endl) continue;
                    for (int i = 0; i < setVals.size(); i++) {
                        Value val = setVals[i];
                        if (!val.type.isRef()) {
                            logError("Can't assign to a value on lhs", nullptr, true);
                            implementScopeRecoverError
                        }
                    }

                    if (setVals.size() == 1) {
                        optional<Value> rval;
                        Value val = setVals[0];
                        if (tokens.getToken().type == tt_addeq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = add(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't add types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_subeq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = sub(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't sub types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_muleq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = mul(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't mul types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_diveq) {
                            Token opEq = tokens.getToken();
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                            optional<Value> newVal = div(val, rval.value());
                            if (!newVal.has_value()) {
                                logError("Can't div types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                                implementScopeRecoverError
                            }
                            rval = newVal;
                        } else if (tokens.getToken().type == tt_eq) {
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                        } else {
                            logError("expected an assignment operator");
                            implementScopeRecoverError
                        }
                        optional<Value> valCast = rval.value().implCast(val.type.dereference());
                        if (!valCast.has_value()) {
                            logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rval.value().type.name, nullptr, true);
                            continue;
                        }
                        val.store(valCast.value());
                    } else {
                        optional<Value> rval;
                        if (tokens.getToken().type == tt_eq) {
                            tokens.nextToken();
                            rval = parseStatment({ tt_lcur }, scope);
                            if (!rval.has_value()) {
                                implementScopeRecoverError
                            }
                        } else {
                            logError("expected an assignment operator");
                            implementScopeRecoverError
                        }
                        if (!rval.value().type.isStruct()) {
                            logError("multiple assignment only work with a struct on the right hand side");
                            implementScopeRecoverError
                        }
                        if (setVals.size() != rval.value().type.elemNames.size()) {
                            logError("right hand side struct contains " + to_string(rval.value().type.elemNames.size()) + " values when setting " + to_string(setVals.size()) + " values", nullptr, true);
                            implementScopeRecoverError
                        }

                        for (int i = 0; i < setVals.size(); i++) {
                            Value val = setVals[i];
                            Value rsval = rval.value().structVal(i);
                            optional<Value> valCast = rsval.implCast(val.type.dereference());
                            if (!valCast.has_value()) {
                                logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rsval.type.name, nullptr, true);
                                continue;
                            }
                            val.store(valCast.value().actualValue());
                        }
                    }
                }

                LLVMBuildBr(builder, whileCon);


                LLVMPositionBuilderAtEnd(builder, whileCon);

                optional<Value> val = branchVal.value().actualValue();
                Value zero = Value(LLVMConstInt(LLVMInt32Type(), 0, 0), nameToType["i32"].front(), this, true);
                optional<Value> zeroAsVal = zero.cast(val.value().type);
                if (!zeroAsVal.has_value() || (!zeroAsVal.value().type.isNumber())) {
                    logError("Expected if statment to have a bool or number as value");
                    implementScopeRecoverError
                }
                LLVMValueRef condition;
                if (zeroAsVal.value().type.isFloat()) {
                    condition = LLVMBuildFCmp(builder, LLVMRealONE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
                } else {
                    condition = LLVMBuildICmp(builder, LLVMIntNE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
                }
                LLVMBuildCondBr(builder, condition, whileBody, merge_block);
            }
            LLVMPositionBuilderAtEnd(builder, whileBody);
            bool scopedBreaks = implementScope(tokens.pos, whileScope, func);
            if (!scopedBreaks) LLVMBuildBr(builder, whileAfter);

            assert(tokens.getToken().type == tt_rcur);
            tokens.nextToken();
            if (tokens.getToken().type != tt_endl) {
                logError("Nothing else should be on same line as }");
                implementScopeRecoverError
            }
            LLVMPositionBuilderAtEnd(builder, merge_block);
            continue;
        }


        //while
        if (tokens.getToken().type == tt_while) {
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func.llvmValue, "merge");
            scope.addBlock(merge_block);
            tokens.nextToken();
            LLVMBasicBlockRef whileCon = LLVMAppendBasicBlock(func.llvmValue, "whileCon");
            LLVMBasicBlockRef whileBody = LLVMAppendBasicBlock(func.llvmValue, "whileBody");
            Scope whileScope(&scope, whileCon, true);
            whileScope.addBlock(whileBody);

            whileScope.gotoFront();
            LLVMPositionBuilderAtEnd(builder, whileCon);
            optional<Value> val = parseStatment({ tt_lcur, tt_endl }, scope);
            if (tokens.getToken().type == tt_endl) {
                logError("Expected { to start scope");
                implementScopeRecoverError
            }
            if (!val.has_value()) {
                implementScopeRecoverError
            }
            val = val.value().actualValue();
            Value zero = Value(LLVMConstInt(LLVMInt32Type(), 0, 0), nameToType["i32"].front(), this, true);
            optional<Value> zeroAsVal = zero.cast(val.value().type);
            if (!zeroAsVal.has_value() || (!zeroAsVal.value().type.isNumber())) {
                logError("Expected if statment to have a bool or number as value");
                implementScopeRecoverError
            }
            LLVMValueRef condition;
            if (zeroAsVal.value().type.isFloat()) {
                condition = LLVMBuildFCmp(builder, LLVMRealONE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
            } else {
                condition = LLVMBuildICmp(builder, LLVMIntNE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
            }
            LLVMBuildCondBr(builder, condition, whileBody, merge_block);

            LLVMPositionBuilderAtEnd(builder, whileBody);
            bool scopedBreaks = implementScope(tokens.pos, whileScope, func);
            if (!scopedBreaks) whileScope.gotoFront();

            assert(tokens.getToken().type == tt_rcur);
            tokens.nextToken();
            if (tokens.getToken().type != tt_endl) {
                logError("Nothing else should be on same line as }");
                implementScopeRecoverError
            }
            LLVMPositionBuilderAtEnd(builder, merge_block);
            continue;
        }



        //if
        if (tokens.getToken().type == tt_if) {
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func.llvmValue, "merge");
            scope.addBlock(merge_block);
            while (true) {
                tokens.nextToken();
                optional<Value> val = parseStatment({ tt_lcur, tt_endl }, scope);
                if (tokens.getToken().type == tt_endl) {
                    logError("Expected { to start scope");
                    implementScopeRecoverError
                }
                if (!val.has_value()) {
                    implementScopeRecoverError
                }
                val = val.value().actualValue();
                Value zero = Value(LLVMConstInt(LLVMInt32Type(), 0, 0), nameToType["i32"].front(), this, true);
                optional<Value> zeroAsVal = zero.cast(val.value().type);
                if (!zeroAsVal.has_value() || (!zeroAsVal.value().type.isNumber())) {
                    logError("Expected if statment to have a bool or number as value");
                    implementScopeRecoverError
                }
                LLVMValueRef condition;
                if (zeroAsVal.value().type.isFloat()) {
                    condition = LLVMBuildFCmp(builder, LLVMRealONE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
                } else {
                    condition = LLVMBuildICmp(builder, LLVMIntNE, zeroAsVal.value().llvmValue, val.value().llvmValue, "cmp");
                }
                LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func.llvmValue, "then");
                LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func.llvmValue, "else");
                LLVMBuildCondBr(builder, condition, then_block, else_block);

                LLVMPositionBuilderAtEnd(builder, then_block);

                Scope ifScope(&scope, then_block, false);
                bool scopedBreaks = implementScope(tokens.pos, ifScope, func);
                if (!scopedBreaks) scope.gotoLast();
                assert(tokens.getToken().type == tt_rcur);
                tokens.nextToken();
                if (tokens.getToken().type != tt_endl) {
                    logError("Nothing else should be on same line as }");
                    implementScopeRecoverError
                }
                tokens.nextToken();

                Scope elseScope(&scope, else_block, false);
                LLVMPositionBuilderAtEnd(builder, else_block);
                if (tokens.getToken().type == tt_else) {
                    tokens.nextToken();
                    if (tokens.getToken().type == tt_lcur) {
                        scopedBreaks = implementScope(tokens.pos, elseScope, func);
                        if (!scopedBreaks) scope.gotoLast();
                        assert(tokens.getToken().type == tt_rcur);
                        tokens.nextToken();
                        if (tokens.getToken().type != tt_endl) {
                            logError("Nothing else should be on same line as }");
                            implementScopeRecoverError
                        }
                        break;
                    } else if (tokens.getToken().type == tt_if) {
                        continue;
                    } else {
                        logError("Expected } after an if statment");
                        implementScopeRecoverError
                    }
                } else {
                    tokens.lastToken();
                    scope.gotoLast();
                    break;
                }
            }

            LLVMPositionBuilderAtEnd(builder, merge_block);
            continue;
        }

        // switch
        if (tokens.getToken().type == tt_switch) {
            tokens.nextToken();
            optional<Value> val = parseStatment({ tt_lcur }, scope);
            if (!val.has_value() || (!val.value().type.isEnum() && val.value().type.actualType().isInt() && val.value().type.actualType().isUInt())) {
                logError("Expected an int, uint, or a enum");
                implementScopeRecoverError
            }
            Value switchNum;
            if (val.value().type.isEnum()) {
                switchNum = val.value().enumType().actualValue();
            } else if (val.value().type.isInt()) {
                switchNum = val.value().actualValue();
            } else if (val.value().type.isUInt()) {
                switchNum = val.value().actualValue();
            } else {
                assert(false);
            }

            vector<Scope> cases;
            vector<i64> caseVals;
            Scope default;
            bool foundDefult = false;
            tokens.nextToken();
            if (tokens.getToken().type != tt_endl) {
                logError("Expected an endline after {");
                implementScopeRecoverError
            }
            tokens.nextToken();
            LLVMBasicBlockRef nextBlock = LLVMAppendBasicBlock(func.llvmValue, "afterSwitch");
            LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
            scope.addBlock(nextBlock);
            bool worked = true;
            while (true) {
                if (tokens.getToken().type == tt_endl) {
                    tokens.nextToken();
                    continue;
                }
                if (tokens.getToken().type == tt_rcur) {
                    break;
                }
                if (tokens.getToken().type == tt_default) {
                    tokens.nextToken();
                    if (tokens.getToken().type != tt_lcur) {
                        logError("Expected {");
                        worked = false;
                        break;
                    }
                    LLVMBasicBlockRef caseBody = LLVMAppendBasicBlock(func.llvmValue, "caseBody");
                    LLVMPositionBuilderAtEnd(builder, caseBody);
                    Scope caseScope(&scope, caseBody, false);
                    default = caseScope;
                    bool exitedScope = implementScope(tokens.pos, caseScope, func);
                    if (!exitedScope) {
                        scope.gotoLast();
                    }
                    foundDefult = true;
                    tokens.nextToken();
                    continue;
                }
                if (tokens.getToken().type == tt_case) {
                    tokens.nextToken();
                }
                LLVMBasicBlockRef caseBody = LLVMAppendBasicBlock(func.llvmValue, "caseBody");
                LLVMPositionBuilderAtEnd(builder, caseBody);
                Scope caseScope(&scope, caseBody, false);
                if (val.value().type.isEnum()) {
                    if (tokens.getToken().type != tt_id) {
                        logError("Expected enum type");
                        worked = false;
                        break;
                    }
                    string enumType = *tokens.getToken().data.str;
                    int enumIndex = -1;
                    for (int i = 0; i < val.value().type.elemNames.size(); i++) {
                        if (val.value().type.elemNames[i] == enumType) {
                            caseVals.push_back(val.value().type.enumValues[i]);
                            enumIndex = i;
                            break;
                        }
                    }
                    if (enumIndex == -1) {
                        logError("Enum doesn't have this value");
                        worked = false;
                        break;
                    }
                    tokens.nextToken();
                    if (val.value().type.enumValues.size() == 0) {
                        break;
                    }
                    if (tokens.getToken().type != tt_lpar) {
                        logError("Expected (");
                        worked = false;
                        break;
                    }
                    tokens.nextToken();
                    vector<Variable> caseVars;
                    LLVMPositionBuilderAtEnd(builder, func.entry);
                    while (true) {
                        if (tokens.getToken().type == tt_rpar) break;
                        optional<Type> type = typeFromTokens();
                        if (!type.has_value()) {
                            logError("Expected type");
                            worked = false;
                            break;
                        }
                        if (tokens.getToken().type != tt_id) {
                            logError("Expected variable name");
                            worked = false;
                            break;
                        }
                        string varName = *tokens.getToken().data.str;
                        Variable var(varName, type.value(), this);
                        caseVars.push_back(var);
                        tokens.nextToken();
                        if (tokens.getToken().type != tt_com && tokens.getToken().type != tt_rpar) {
                            logError("Expected , or )");
                            worked = false;
                            break;
                        }
                        if (tokens.getToken().type == tt_com) tokens.nextToken();
                    }
                    if (!worked) {
                        break;
                    }
                    tokens.nextToken();
                    if (tokens.getToken().type != tt_lcur) {
                        logError("Expected {");
                        worked = false;
                        break;
                    }
                    LLVMPositionBuilderAtEnd(builder, caseBody);
                    if (caseVars.size() == 1) {
                        if (caseVars[0].value.type.actualType() != val.value().enumVal(enumIndex).type.actualType()) {
                            logError("Expected type to match enum types");
                            worked = false;
                            break;
                        }
                        caseVars[0].store(val.value().enumVal(enumIndex).actualValue());
                        caseScope.addVariable(caseVars[0]);
                    } else {
                        if (val.value().enumVal(enumIndex).type.elemTypes.size() != caseVars.size()) {
                            logError("Expected number of variables to match number of enum values");
                            worked = false;
                            break;
                        }
                        for (int i = 0; i < val.value().enumVal(enumIndex).type.elemTypes.size(); i++) {
                            optional<Value> valCast = val.value().enumVal(enumIndex).structVal(i).implCast(caseVars[i].value.type.dereference());
                            if (!valCast.has_value()) {
                                logError("Type of assignment and value don't match. Assignment Type: " + caseVars[i].value.type.dereference().name + " | Value Type: " + val.value().enumVal(enumIndex).structVal(i).type.name,
                                    nullptr, true);
                                continue;
                            }
                            caseVars[i].store(valCast.value());
                            caseScope.addVariable(caseVars[i]);
                        }
                    }
                } else {
                    if (tokens.getToken().type != tt_int) {
                        logError("Expected value");
                        worked = false;
                        break;
                    }
                    caseVals.push_back(tokens.getToken().data.uint);
                    tokens.nextToken();
                }
                if (tokens.getToken().type != tt_lcur) {
                    logError("Expected {");
                    worked = false;
                    break;
                }
                bool exitedScope = implementScope(tokens.pos, caseScope, func);
                cases.push_back(caseScope);
                if (!exitedScope) {
                    scope.gotoLast();
                }
                tokens.nextToken();
            }
            if (!worked) {
                implementScopeRecoverError
            }
            if (!foundDefult) {
                logError("never found default in case statment");
                implementScopeRecoverError
            }
            //todo all this
            LLVMPositionBuilderAtEnd(builder, curBlock);

            LLVMValueRef switchInst = LLVMBuildSwitch(builder, switchNum.llvmValue, default.blocks.front(), cases.size());  // 2 cases

            // Add cases
            for (int i = 0; i < cases.size(); i++) {
                LLVMAddCase(switchInst, LLVMConstInt(switchNum.type.llvmType, caseVals[i], 0), cases[i].blocks.front());
            }
            LLVMPositionBuilderAtEnd(builder, nextBlock);
            tokens.nextToken();
            if (tokens.getToken().type != tt_endl) {
                logError("Expected endl");
                implementScopeRecoverError
            }
            continue;
        }

        // return
        if (tokens.getToken().type == tt_ret) {
            tokens.nextToken();
            if (tokens.getToken().type == tt_endl) {
                if (func.returnType.name != "void") {
                    if (func.name == "main") {
                        LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
                        tokens.nextToken();
                        tokens.nextToken();
                        if (tokens.getToken().type != tt_rcur) {
                            logError("returned last line so expected }");
                            implementScopeRecoverError
                        }
                        return true;
                    }
                    logError("Expected value to return");
                    continue;
                }
                LLVMBuildRetVoid(builder);
                tokens.nextToken();
                if (tokens.getToken().type != tt_rcur) {
                    logError("returned last line so expected }");
                    implementScopeRecoverError
                }
                return true;
            }
            if (func.returnType.name == "void") {
                logError("Expected endline for return from void funciton");
                implementScopeRecoverError
            }
            optional<Value> val = parseStatment({ tt_endl }, scope);
            if (!val.has_value()) {
                implementScopeRecoverError
            }
            if (func.returnType.isRef()) {
                if (func.returnType != val.value().type) {
                    logError("Type of return and value don't match. return type: " + func.returnType.name + " | Value Type: " + val.value().type.name, nullptr, true);
                    continue;
                }
                LLVMBuildRet(builder, val.value().llvmValue);
            } else {
                optional<Value> valCast = val.value().implCast(func.returnType.actualType());
                if (!valCast.has_value()) {
                    logError("Type of return and value don't match. return type: " + func.returnType.actualType().name + " | Value Type: " + val.value().type.actualType().name, nullptr, true);
                    continue;
                }
                LLVMBuildRet(builder, valCast.value().llvmValue);
            }
            tokens.nextToken();
            if (tokens.getToken().type != tt_rcur) {
                logError("returned last line so expected }");
                implementScopeRecoverError
            }
            return true;
        }



        vector<Value> setVals;
        bool worked = true;
        while (true) {
            optional<Type> type = typeFromTokens(false);
            if (type.has_value()) {
                // type
                if (tokens.getToken().type != tt_id) {
                    logError("Expected variable name");
                    worked = false;
                    break;
                }
                string varName = *tokens.getToken().data.str;
                LLVMBasicBlockRef curBlock = LLVMGetInsertBlock(builder);
                LLVMPositionBuilderAtEnd(builder, func.entry);
                bool worked = scope.addVariable(Variable(varName, type.value(), this));
                LLVMPositionBuilderAtEnd(builder, curBlock);
                if (!worked) {
                    logError("Already have a variable with this name in scope");
                    worked = false;
                    break;
                }
                Variable* var = scope.getVariableFromName(varName).value();
                Value val = var->value;
                setVals.push_back(val);
                tokens.nextToken();
            } else {
                // statment
                optional<Value> val = parseStatment({ tt_eq, tt_addeq, tt_subeq, tt_muleq, tt_diveq, tt_endl, tt_com }, scope);
                if (!val.has_value()) {
                    worked = false;
                    break;
                }
                setVals.push_back(val.value());
            }
            if (tokens.getToken().type == tt_com) {
                tokens.nextToken();
                continue;
            }
            break;
        }
        if (!worked) {
            implementScopeRecoverError
        }
        if (tokens.getToken().type == tt_endl) continue;
        for (int i = 0; i < setVals.size(); i++) {
            Value val = setVals[i];
            if (!val.type.isRef()) {
                logError("Can't assign to a value on lhs", nullptr, true);
                implementScopeRecoverError
            }
        }

        if (setVals.size() == 1) {
            optional<Value> rval;
            Value val = setVals[0];
            if (tokens.getToken().type == tt_addeq) {
                Token opEq = tokens.getToken();
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
                optional<Value> newVal = add(val, rval.value());
                if (!newVal.has_value()) {
                    logError("Can't add types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                    implementScopeRecoverError
                }
                rval = newVal;
            } else if (tokens.getToken().type == tt_subeq) {
                Token opEq = tokens.getToken();
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
                optional<Value> newVal = sub(val, rval.value());
                if (!newVal.has_value()) {
                    logError("Can't sub types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                    implementScopeRecoverError
                }
                rval = newVal;
            } else if (tokens.getToken().type == tt_muleq) {
                Token opEq = tokens.getToken();
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
                optional<Value> newVal = mul(val, rval.value());
                if (!newVal.has_value()) {
                    logError("Can't mul types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                    implementScopeRecoverError
                }
                rval = newVal;
            } else if (tokens.getToken().type == tt_diveq) {
                Token opEq = tokens.getToken();
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
                optional<Value> newVal = div(val, rval.value());
                if (!newVal.has_value()) {
                    logError("Can't div types of " + val.type.name + " with " + rval.value().type.name, &opEq);
                    implementScopeRecoverError
                }
                rval = newVal;
            } else if (tokens.getToken().type == tt_eq) {
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
            } else {
                logError("expected an assignment operator");
                implementScopeRecoverError
            }
            optional<Value> valCast = rval.value().implCast(val.type.dereference());
            if (!valCast.has_value()) {
                logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rval.value().type.name, nullptr, true);
                continue;
            }
            val.store(valCast.value());
        } else {
            optional<Value> rval;
            if (tokens.getToken().type == tt_eq) {
                tokens.nextToken();
                rval = parseStatment({ tt_endl }, scope);
                if (!rval.has_value()) {
                    implementScopeRecoverError
                }
            } else {
                logError("expected an assignment operator");
                implementScopeRecoverError
            }
            if (!rval.value().type.isStruct()) {
                logError("multiple assignment only work with a struct on the right hand side");
                implementScopeRecoverError
            }
            if (setVals.size() != rval.value().type.elemNames.size()) {
                logError("right hand side struct contains " + to_string(rval.value().type.elemNames.size()) + " values when setting " + to_string(setVals.size()) + " values", nullptr, true);
                implementScopeRecoverError
            }

            for (int i = 0; i < setVals.size(); i++) {
                Value val = setVals[i];
                Value rsval = rval.value().structVal(i);
                optional<Value> valCast = rsval.implCast(val.type.dereference());
                if (!valCast.has_value()) {
                    logError("Type of assignment and value don't match. Assignment Type: " + val.type.dereference().name + " | Value Type: " + rsval.type.name, nullptr, true);
                    continue;
                }
                val.store(valCast.value().actualValue());
            }
        }

        continue;
    }
    if (scope.parent == nullptr) {
        return false;
    }
    return false;
}

void Module::implementFunction(TokenPositon start, Function& func) {
    if (func.external) return;
    tokens.pos = start;

    while (true) {
        if (tokens.getToken().type == tt_lcur) break;
        tokens.nextToken();
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func.llvmValue, "entry");
    LLVMBasicBlockRef body = LLVMAppendBasicBlock(func.llvmValue, "body");
    func.scope = Scope(nullptr, entry, false);
    func.entry = entry;
    LLVMPositionBuilderAtEnd(builder, entry);
    func.scope.addBlock(body);
    if (func.methodOfType.has_value() && func.methodOfType.value().isStruct()) {
        for (int i = 0; i < func.methodOfType.value().elemNames.size(); i++) {
            Variable var(func.methodOfType.value().elemNames[i], func.methodOfType.value().elemTypes[i], func.getParamValue(0).structVal(i), this);
            bool worked = func.scope.addVariable(var);
            if (!worked) {
                logError("Two parameters have the same name", nullptr, true);
                return;
            }
        }
    }
    for (int j = 0; j < func.paramNames.size(); j++) {
        Variable var(func.paramNames[j], func.paramTypes[j], func.getParamValue(j), this);
        bool worked = func.scope.addVariable(var);
        if (!worked) {
            logError("Two parameters have the same name", nullptr, true);
            return;
        }
    }


    LLVMPositionBuilderAtEnd(builder, body);
    implementScope(tokens.pos, func.scope, func);

    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMBuildBr(builder, body);
}

bool Module::implementScope(TokenPositon start, Scope& scope, Function& func) {
    bool validScopeExit = implementScopeHelper(start, scope, func);
    if (!validScopeExit) {
        if (scope.parent == nullptr) {
            if (func.name == "main") {
                LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
            } else if (func.returnType.name == "void") {
                LLVMBuildRetVoid(builder);
            } else {
                logError("didn't return from function outer most scope");
                return false;
            }
        } else {
            return false;
        }
    }
    //Todo: end of scope op like defer
    return true;
}


bool Module::looksLikeType() {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_id) {
        tokens.pos = start;
        return false;
    }
    tokens.nextToken();
    while (true) {
        switch (tokens.getToken().type) {
            case tt_mul: {
                tokens.nextToken();
                break;
            }
            case tt_and: {
                tokens.nextToken();
                break;
            }
            case tt_lbar: {
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    tokens.pos = start;
                    return false;
                }
                tokens.nextToken();
                if (tokens.getToken().type != tt_rbar) {
                    tokens.pos = start;
                    return false;
                }
                tokens.nextToken();
                break;
            }
            case tt_bitor: {
                tokens.nextToken();
                if (!looksLikeType()) {
                    tokens.pos = start;
                    return false;
                }
                return true;
            }
            case tt_com: {
                tokens.nextToken();
                if (!looksLikeType()) {
                    tokens.pos = start;
                    return false;
                }
                return true;
            }
            default: {
                return true;
                break;
            }
        }
    }
}

string Module::dirName() {
    stringstream ss;
    int lastSlash = 0;
    for (int i = 0; i < dir.size(); i++) {
        if (dir[i] == '\\' || dir[i] == '/') lastSlash = i;
    }
    for (int i = lastSlash + 1; i < dir.size(); i++) {
        ss << dir[i];
    }
    return ss.str();
}

bool Module::looksLikeFunction() {
    TokenPositon start = tokens.pos;
    if (!looksLikeType()) return false;
    TokenPositon typeEnd = tokens.pos;
    if (looksLikeType()) {
        if (tokens.getToken().type != tt_dot) {
            tokens.pos = typeEnd;
        } else {
            tokens.nextToken();
        }
    }
    if (tokens.getToken().type != tt_id) {
        tokens.pos = start;
        return false;
    }
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
    if (tokens.getToken().type == tt_endl) {
        return true;
    }
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
bool Module::looksLikeEnum() {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_enum) return false;
    tokens.nextToken();
    if (tokens.getToken().type != tt_id) {
        tokens.pos = start;
        return false;
    }
    tokens.nextToken();
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

bool Module::looksLikeStruct() {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_struct) return false;
    tokens.nextToken();
    if (tokens.getToken().type != tt_id) {
        tokens.pos = start;
        return false;
    }
    tokens.nextToken();
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
