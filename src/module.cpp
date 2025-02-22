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

optional<Type> Module::typeFromTokens(bool logErrors) {
    TokenPositon start = tokens.pos;
    if (tokens.getToken().type != tt_id) {
        if (logErrors) logError("Expected type");
        return nullopt;
    }
    string name = *tokens.getToken().data.str;
    auto t = nameToType.find(name);
    if (t == nameToType.end()) {
        if (logErrors) logError("Type doesn't exist");
        return nullopt;
    }
    if (t->second.size() != 1) {
        if (logErrors) logError("Ambiguous type");
    }
    Type type = t->second[0];
    tokens.nextToken();
    while (true) {
        switch (tokens.getToken().type) {
            case tt_mul: {
                type = type.ptr();
                tokens.nextToken();
                break;
            }
            case tt_and: {
                type = type.ref();
                tokens.nextToken();
                break;
            }
            case tt_lbar: {
                tokens.nextToken();
                if (tokens.getToken().type != tt_int) {
                    logError("Expected number");
                    tokens.pos = start;
                    return nullopt;
                }
                u64 number = tokens.getToken().data.uint;
                tokens.nextToken();
                if (tokens.getToken().type != tt_rbar) {
                    logError("Expected ]");
                    tokens.pos = start;
                    return nullopt;
                }
                tokens.nextToken();
                type = type.vec(number);
                break;
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
            case tt_add: {
                if (prio >= 3) return lval;
                tokens.nextToken();
                optional<Value> rval = parseValue(scope);
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
            string name = *tokens.getToken().data.str;
            tokens.nextToken();
            if (tokens.getToken().type == tt_lpar) {
                optional<Value> val = parseFunctionCall(name, scope);
                if (!val.has_value()) {
                    tokens.pos = start;
                    return nullopt;
                }
                return val.value();
            }
            optional<Variable*> var = scope.getVariableFromName(name);
            if (!var.has_value()) {
                logError("No variable with this name", &s);
                tokens.pos = start;
                return nullopt;
            }
            return var.value()->value;
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

            LLVMValueRef strAlloca = LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8TypeInContext(context), str.size() + 1), "str");

            for (size_t i = 0; i < str.size() + 1; i++) {
                LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(context), i, false);
                LLVMValueRef charPtr = LLVMBuildGEP2(builder, LLVMInt8TypeInContext(context), strAlloca, &index, 1, "charPtr");
                LLVMBuildStore(builder, LLVMConstInt(LLVMInt8TypeInContext(context), str[i], false), charPtr);
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
    return optional<Value>();
}

optional<Value> Module::parseFunctionCall(string& name, Scope& scope) {

    tokens.lastToken();
    Token funcNameToken = tokens.getToken();
    tokens.nextToken();

    TokenPositon start = tokens.pos;
    vector<Value> vals;
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

    return funcToCall->call(vals, this);
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

    nameToFunction[funcName].push_back(Function(type.value(), funcName, paramTypes, paramNames, this, variadicArgs, external));
    return &nameToFunction[funcName].back();
}

void Module::implementScope(TokenPositon start, Scope& scope, Function& func) {
    tokens.pos = start;
    assert(tokens.getToken().type == tt_lcur);
    tokens.nextToken();

    if (tokens.getToken().type != tt_endl) {
        logError("Nothing else should be on this line");
        while (true) {
            if (tokens.getToken().type == tt_endl) break;
            if (tokens.getToken().type == tt_rcur) return;
            tokens.nextToken();
        }
    }

    while (true) {
        assert(tokens.getToken().type == tt_endl);
        tokens.nextToken();
        bool err = false;
        if (tokens.getToken().type == tt_rcur) {
            tokens.nextToken();
            break;
        }

        optional<Type> type = typeFromTokens(false);
        if (type.has_value()) {
            if (tokens.getToken().type != tt_id) {
                logError("Expected variable name");
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            }
            string varName = *tokens.getToken().data.str;
            bool worked = scope.addVariable(Variable(varName, type.value(), this));
            if (!worked) {
                logError("Already have a variable with this name in scope");
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            }
            tokens.nextToken();
            if (tokens.getToken().type == tt_endl) continue;
            if (tokens.getToken().type == tt_eq) {
                tokens.nextToken();
                optional<Value> val = parseStatment({ tt_endl }, scope);
                if (val.has_value()) {
                    Variable* var = scope.getVariableFromName(varName).value();
                    optional<Value> valCast = val.value().implCast(var->value.type.actualType());
                    if (valCast.has_value()) {
                        var->store(valCast.value());
                        continue;
                    } else {
                        logError("Type of assignment and value don't match. Assignment Type: " + var->value.type.actualType().name + " | Value Type: " + val.value().type.name, nullptr, true);
                    }
                    continue;
                }
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            } else {
                logError("Expected an assinment or new line after variable declaration");
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            }
        } else {
            if (tokens.getToken().type == tt_ret) {
                tokens.nextToken();
                if (tokens.getToken().type == tt_endl) {
                    if (func.returnType.name != "void") {
                        logError("Expected value to return");
                        continue;
                    }
                    LLVMBuildRetVoid(builder);
                    continue;
                }
                if (func.returnType.name == "void") {
                    logError("Expected endline for return from void funciton");
                    while (true) {
                        if (tokens.getToken().type == tt_endl) break;
                        if (tokens.getToken().type == tt_rcur) return;
                        tokens.nextToken();
                    }
                    continue;
                }
                optional<Value> val = parseStatment({ tt_eq, tt_endl }, scope);
                if (!val.has_value()) {
                    while (true) {
                        if (tokens.getToken().type == tt_endl) break;
                        if (tokens.getToken().type == tt_rcur) return;
                        tokens.nextToken();
                    }
                    continue;
                }
                if (func.returnType.isRef()) {
                    if (func.returnType != val.value().type) {
                        logError("Type of return and value don't match. return type: " + func.returnType.name + " | Value Type: " + val.value().type.name, nullptr, true);
                        continue;
                    }
                    LLVMBuildRet(builder, val.value().llvmValue);
                    continue;
                } else {
                    optional<Value> valCast = val.value().implCast(func.returnType.actualType());
                    if (!valCast.has_value()) {
                        logError("Type of return and value don't match. return type: " + func.returnType.actualType().name + " | Value Type: " + val.value().type.actualType().name, nullptr, true);
                        continue;
                    }
                    LLVMBuildRet(builder, valCast.value().llvmValue);
                    continue;
                }
            }
            optional<Value> val = parseStatment({ tt_eq, tt_endl }, scope);
            if (!val.has_value() || !val.value().type.isRef()) {
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            }
            if (tokens.getToken().type == tt_endl) continue;
            assert(tokens.getToken().type == tt_eq);
            tokens.nextToken();
            optional<Value> rval = parseStatment({ tt_endl }, scope);
            if (!rval.has_value()) {
                while (true) {
                    if (tokens.getToken().type == tt_endl) break;
                    if (tokens.getToken().type == tt_rcur) return;
                    tokens.nextToken();
                }
                continue;
            }
            optional<Value> valCast = rval.value().implCast(val.value().type.actualType());
            if (!valCast.has_value()) {
                logError("Type of assignment and value don't match. Assignment Type: " + val.value().type.actualType().name + " | Value Type: " + rval.value().type.actualType().name, nullptr, true);
                continue;
            }
            val.value().store(valCast.value().actualValue());
            continue;
        }
    }
}

void Module::implementFunction(TokenPositon start, Function& func) {
    if (func.external) return;
    tokens.pos = start;

    while (true) {
        if (tokens.getToken().type == tt_lcur) break;
        tokens.nextToken();
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func.llvmValue, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    for (int j = 0; j < func.paramNames.size(); j++) {
        Variable var(func.paramNames[j], func.paramTypes[j], this);
        var.store(func.getParamValue(j));
        bool worked = func.scope.addVariable(var);
        if (!worked) {
            logError("Two parameters have the same name", nullptr, true);
            return;
        }
    }

    implementScope(tokens.pos, func.scope, func);
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
