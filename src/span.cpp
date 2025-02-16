#include "span.h"
#include <filesystem>
#include <fstream>
#include <assert.h>

using namespace std;
namespace fs = std::filesystem;

LLVMContextRef context;
LLVMBuilderRef builder;
LLVMModuleRef llvmModule;
LLVMTargetDataRef dataLayout;
std::vector<std::string> files;
std::vector<std::vector<std::string>> textByFileByLine;
std::vector<Token> tokens;
std::vector<u64> functionStarts;
unordered_map<string, Type> nameToType;
unordered_map<string, vector<Function>> nameToFunction;
bool hadError = false;

Variable dummyVar = {};

Value parseStatment(int& i, bool& err, Scope& scope, const vector<TokenType>& dels, int retPrio = INT_MIN);

string loadFileToString(const string& filePath) {
    ifstream file(filePath, ios::in | ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filePath << '\n';
        return "";
    }
    auto fileSize = file.tellg();
    string content(fileSize, '\0');
    file.seekg(0);
    file.read(&content[0], fileSize);
    return content;
}

vector<string> splitStringByNewline(const std::string& str) {
    vector<string> lines;
    istringstream stream(str);
    string line;
    lines.push_back("");

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
}

string removeSpaces(const string& str) {
    stringstream ss;
    bool startSpaces = true;
    for (int i = 0; i < str.size(); i++) {
        if (isspace(str[i]) && startSpaces) continue;
        ss << str[i];
        startSpaces = false;
    }
    return ss.str();
}

void logError(const string& err, Token& token, bool wholeLine = false) {
    hadError = true;
    std::cout << "\033[31m";
    cout << "Error: " << err << endl;
    std::cout << "\033[0m";
    cout << removeSpaces(textByFileByLine[token.file][token.line]) << endl;
    std::cout << "\033[31m";
    if (wholeLine) {
        for (int i = 0; i <= textByFileByLine[token.file][token.line].size(); i++) {
            cout << "^";
        }
    } else {
        bool startSpaces = true;
        for (int i = 0; i < token.schar; i++) {
            if (isspace(textByFileByLine[token.file][token.line][i]) && startSpaces) {
                continue;
            }
            cout << " ";
            startSpaces = false;
        }
        for (int i = token.schar; i <= token.echar; i++) {
            cout << "^";
        }
    }
    std::cout << "\033[0m";
    cout << endl;
    cout << "Line: " << token.line << " | File: " << files[token.file] << endl;
    cout << "-------------------------------------" << endl;
}

void getTokens(u8 file) {
    for (u16 lineNum = 0; lineNum < textByFileByLine[file].size(); lineNum++) {
        string& line = textByFileByLine[file][lineNum];
        for (u16 c = 0; c < line.size(); c++) {
            while (c < line.size() && (isspace(line[c]) || line[c] == '\0'))
                c++;
            if (c >= line.size()) break;
            Token token;
            token.schar = c;
            token.line = lineNum;
            token.file = file;

            switch (line[c]) {
                case '_':
                CASELETTER: {
                    stringstream ss;
                    bool done = false;
                    while (c < line.size() && !done) {
                        switch (line[c]) {
                            case '_':
                            CASELETTER:
                            CASENUMBER: {
                                ss << line[c];
                                c++;
                                break;
                            }
                            default: {
                                done = true;
                                break;
                            }
                        }
                    }
                    c--;
                    token.data.str = new string;
                    *token.data.str = ss.str();
                    token.echar = c;
                    token.type = tt_id;
                    break;
                }
                case '"': {
                    c++;
                    stringstream ss;
                    bool done = false;
                    while (c < line.size() && !done) {
                        switch (line[c]) {
                            case '"': {
                                done = true;
                                break;
                            }
                            default: {
                                ss << line[c];
                                c++;
                                break;
                            }
                        }
                    }
                    if (!done) {
                        token.echar = c;
                        token.type = tt_err;
                        break;
                    }
                    c--;
                    string str = ss.str();
                    token.echar = c;

                    if (str == "return") {
                        token.type = tt_ret;
                        break;
                    }
                    // Add more keywords here

                    token.data.str = new string;
                    *token.data.str = str;
                    token.type = tt_str;
                    break;
                }
                case '.':
                CASENUMBER: {
                    bool isFloat = false;
                    stringstream ss;
                    while (c < line.size() && (isdigit(line[c]) || line[c] == '.')) {
                        ss << line[c];
                        if (line[c] == '.') isFloat = true;
                        c++;
                    }
                    c--;
                    token.echar = c;
                    if (token.schar == token.echar && line[c] == '.') {
                        token.type = tt_err;
                        break;
                    }
                    token.type = isFloat ? tt_float : tt_int;
                    if (isFloat) {
                        token.data.dec = std::stod(ss.str());
                    } else {
                        token.data.uint = std::stoull(ss.str());
                    }
                    break;
                }
                case '=': {
                    if (c + 1 < line.size() && line[c + 1] == '=') {
                        c++;
                        token.type = tt_eqeq;
                        token.echar = c;
                        break;
                    }
                    token.type = tt_eq;
                    token.echar = c;
                    break;
                }
                case '|': {
                    if (c + 1 < line.size() && line[c + 1] == '|') {
                        c++;
                        token.type = tt_oror;
                        token.echar = c;
                        break;
                    }
                    token.type = tt_or;
                    token.echar = c;
                    break;
                }
                case '&': {
                    if (c + 1 < line.size() && line[c + 1] == '&') {
                        c++;
                        token.type = tt_andand;
                        token.echar = c;
                        break;
                    }
                    token.type = tt_and;
                    token.echar = c;
                    break;
                }
                case '<': {
                    if (c + 1 < line.size() && line[c + 1] == '=') {
                        c++;
                        token.type = tt_leeq;
                        token.echar = c;
                        break;
                    }
                    token.type = tt_le;
                    token.echar = c;
                    break;
                }
                case '>': {
                    if (c + 1 < line.size() && line[c + 1] == '=') {
                        c++;
                        token.type = tt_greq;
                        token.echar = c;
                        break;
                    }
                    token.type = tt_gr;
                    token.echar = c;
                    break;
                }
                case '+': {
                    token.type = tt_add;
                    token.echar = c;
                    break;
                }
                case '-': {
                    token.type = tt_sub;
                    token.echar = c;
                    break;
                }
                case '*': {
                    token.type = tt_mul;
                    token.echar = c;
                    break;
                }
                case '/': {
                    token.type = tt_div;
                    token.echar = c;
                    break;
                }
                case '(': {
                    token.type = tt_lpar;
                    token.echar = c;
                    break;
                }
                case ')': {
                    token.type = tt_rpar;
                    token.echar = c;
                    break;
                }
                case '{': {
                    token.type = tt_lcur;
                    token.echar = c;
                    break;
                }
                case '}': {
                    token.type = tt_rcur;
                    token.echar = c;
                    break;
                }
                case '[': {
                    token.type = tt_lbar;
                    token.echar = c;
                    break;
                }
                case ']': {
                    token.type = tt_rbar;
                    token.echar = c;
                    break;
                }
                case '^': {
                    token.type = tt_car;
                    token.echar = c;
                    break;
                }
                case ',': {
                    token.type = tt_com;
                    token.echar = c;
                    break;
                }
                default: {
                    token.echar = c;
                    token.type = tt_err;
                    break;
                }
            }
            tokens.push_back(token);
        }
        Token eof;
        eof.type = tt_endl;
        eof.schar = line.size();
        eof.echar = line.size();
        eof.line = lineNum;
        eof.file = file;
        tokens.push_back(eof);
    }
    Token eof;
    eof.type = tt_eof;
    eof.schar = UINT16_MAX;
    eof.echar = UINT16_MAX;
    eof.line = UINT16_MAX;
    eof.file = file;
    tokens.push_back(eof);
}

bool nextToken(int& i) {
    return ++i >= tokens.size();
}

bool looksLikeType(int& i) {
    int si = i;
    if (tokens[si].type != tt_id) return false;
    if (nextToken(si)) return false;
    // TODO: Type mod

    i = si;
    return true;
}

bool looksLikeFunction(int& i) {
    int si = i;
    if (!looksLikeType(si)) return false;
    if (tokens[si].type != tt_id) return false;
    if (nextToken(si)) return false;
    if (tokens[si].type != tt_lpar) return false;
    while (true) {
        if (nextToken(si)) return false;
        if (tokens[si].type == tt_rpar) break;
    }
    if (nextToken(si)) return false;
    if (tokens[si].type != tt_lcur) return false;
    int curStack = 1;
    while (curStack != 0) {
        if (nextToken(si)) return false;
        if (tokens[si].type == tt_lcur) curStack++;
        if (tokens[si].type == tt_rcur) curStack--;
    }
    i = si;
    return true;
}

void findFunctionStarts() {
    for (int i = 0; i < tokens.size(); i++) {
        int s = i;
        if (looksLikeFunction(i)) {
            functionStarts.push_back(s);
        } else if (tokens[i].type == tt_endl) {
            continue;
        } else if (tokens[i].type == tt_eof) {
            continue;
        } else {
            logError("Don't know what this top level line is", tokens[i], true);
            while (true) {
                if (tokens[i].type == tt_endl) break;
                if (tokens[i].type == tt_eof) break;
                if (tokens[i].type == tt_lcur) {
                    int curStack = 1;
                    while (curStack != 0) {
                        if (nextToken(i)) return;
                        if (tokens[i].type == tt_lcur) curStack--;
                        if (tokens[i].type == tt_rcur) curStack++;
                    }
                    if (nextToken(i)) return;
                    if (tokens[i].type == tt_endl) break;
                    if (tokens[i].type == tt_eof) break;
                    logError("Right brackets should be on there own line", tokens[i]);
                }
                if (nextToken(i)) return;
            }
        }
    }
}

Type createType(const string& name, LLVMTypeRef llvmType) {
    Type type;
    type.name = name;
    type.llvmType = llvmType;
    nameToType[name] = type;
    return type;
}

void aliasType(const string& name, Type typeToAlias) {
    nameToType[name] = typeToAlias;
}

bool isIntType(const Type& type) {
    LLVMTypeKind typeKind = LLVMGetTypeKind(type.llvmType);
    return typeKind == LLVMIntegerTypeKind;
}

bool isIntSigned(const Type& type) {
    return type.name[0] == 'i';
}

bool isFloatType(const Type& type) {
    LLVMTypeKind typeKind = LLVMGetTypeKind(type.llvmType);
    return typeKind == LLVMDoubleTypeKind || typeKind == LLVMFloatTypeKind || typeKind == LLVMHalfTypeKind;
}

bool isNumType(const Type& type) {
    return isFloatType(type) || isIntType(type);
}

int getTypeWidth(const Type& type) {
    return LLVMSizeOfTypeInBits(dataLayout, type.llvmType);
}

bool typeMatch(Type type1, Type type2) {
    return type1.name == type2.name;
}

Type getTypeFromName(const string& name, bool& err) {
    auto t = nameToType.find(name);
    if (t == nameToType.end()) {
        err = true;
        return {};
    }
    return t->second;
}

Type getType(int& i, bool& err, bool logErrors = true) {
    int si = i;
    if (tokens[si].type != tt_id) {
        if (logErrors) logError("Expected a type name", tokens[i]);
        err = true;
        return {};
    }
    string baseTypeName = *tokens[i].data.str;
    Type baseType = getTypeFromName(baseTypeName, err);
    if (err) {
        if (logErrors) logError("There is no type with name of " + baseTypeName, tokens[i]);
        return {};
    }
    nextToken(si);
    // TODO: type mods

    i = si;
    return baseType;
}

bool isTypePtr(const Type& type) {
    return type.name.back() == '*';
}

bool isTypeRef(Type type) {
    return type.name.back() == '&';
}

Type getTypePtr(Type type) {
    type.name = type.name + '*';
    type.llvmType = LLVMPointerType(type.llvmType, 0);
    return type;
}

Type getTypeRef(Type type) {
    type.name = type.name + '&';
    type.llvmType = LLVMPointerType(type.llvmType, 0);
    return type;
}


Type getTypePointingTo(Type type, bool& err) {
    if (type.name.back() != '*' && type.name.back() != '&') {
        err = true;
        return {};
    }
    type.name.resize(type.name.size() - 1);
    return getTypeFromName(type.name, err);
}

Value dereferenceValue(Value val) {
    assert(isTypePtr(val.type) || isTypeRef(val.type));
    bool err = false;
    val.type = getTypePointingTo(val.type, err);
    assert(!err);
    val.llvmVal = LLVMBuildLoad2(builder, val.type.llvmType, val.llvmVal, "derefVal");
    return val;
}

void prototypeFunction(int i) {
    int s = i;
    bool err = false;
    Type returnType = getType(i, err);
    if (err) return;
    if (tokens[i].type != tt_id) {
        logError("Expected a function name", tokens[i]);
        return;
    }
    string funcName = *tokens[i].data.str;
    nextToken(i);
    nextToken(i);
    vector<Type> paramTypes;
    vector<string> paramNames;
    if (tokens[i].type != tt_rpar) {
        while (true) {
            Type paramType = getType(i, err);
            if (err) return;
            if (tokens[i].type != tt_id) {
                logError("Expected a parameter name", tokens[i]);
                return;
            }
            paramTypes.push_back(paramType);
            paramNames.push_back(*tokens[i].data.str);
            nextToken(i);
            if (tokens[i].type == tt_rpar) break;
            if (tokens[i].type != tt_com) {
                logError("Expected a comma , or closing paren )", tokens[i]);
                return;
            }
            nextToken(i);
        }
    }

    vector<LLVMTypeRef> llvmParamTypes(paramNames.size());
    for (int j = 0; j < paramTypes.size(); j++) {
        llvmParamTypes[j] = paramTypes[j].llvmType;
    }
    Function func;
    func.name = funcName;
    func.paramNames = paramNames;
    func.paramTypes = paramTypes;
    func.returnType = returnType;
    func.llvmType = LLVMFunctionType(returnType.llvmType, llvmParamTypes.data(), llvmParamTypes.size(), 0);
    string llvmname = funcName + to_string(nameToFunction[funcName].size());
    func.llvmValue = LLVMAddFunction(llvmModule, llvmname.c_str(), func.llvmType);
    func.tokenPos = s;
    nameToFunction[funcName].push_back(func);
}

Variable& getVariableFromName(const string& name, Scope& scope, bool& err) {
    Scope* s = &scope;
    while (s != nullptr) {
        auto t = s->nameToVariable.find(name);
        if (t != s->nameToVariable.end()) return t->second;
        s = s->parent.get();
    }
    err = true;
    return dummyVar;
}

void implicitCastNumber(Type& type, Value& val) {
    int typeWidth = 0;
    int valWidth = 0;
    if (isIntType(type)) {
        typeWidth = LLVMGetIntTypeWidth(type.llvmType);
    } else {
        if (type.name == "f16") typeWidth = 16;
        if (type.name == "f32") typeWidth = 32;
        if (type.name == "f64") typeWidth = 64;
    }
    if (isIntType(val.type)) {
        valWidth = LLVMGetIntTypeWidth(val.type.llvmType);
    } else {
        if (val.type.name == "f16") valWidth = 16;
        if (val.type.name == "f32") valWidth = 32;
        if (val.type.name == "f64") valWidth = 64;
    }

    if (isFloatType(type)) {
        if (isFloatType(val.type)) {
            val.llvmVal = LLVMBuildFPCast(builder, val.llvmVal, type.llvmType, "floatTofloat");
        } else {
            if (isIntSigned(val.type)) {
                val.llvmVal = LLVMBuildSIToFP(builder, val.llvmVal, type.llvmType, "intToFloat");
            } else {
                val.llvmVal = LLVMBuildUIToFP(builder, val.llvmVal, type.llvmType, "intToFloat");
            }
        }
    } else {
        if (typeWidth > valWidth) {
            if (isIntSigned(type)) {
                val.llvmVal = LLVMBuildSExt(builder, val.llvmVal, type.llvmType, "intToint");
            } else {
                val.llvmVal = LLVMBuildZExt(builder, val.llvmVal, type.llvmType, "intToint");
            }
        } else {
            val.llvmVal = LLVMBuildTrunc(builder, val.llvmVal, type.llvmType, "intToint");
        }
    }
    val.type = type;
}

Value callFunction(const string& name, int& i, bool& err, Scope& scope) {
    int si = i;
    assert(tokens[si].type == tt_lpar);
    nextToken(si);
    vector<Value> vals;
    if (tokens[si].type != tt_rpar) {
        while (true) {
            Value val = parseStatment(si, err, scope, { tt_com, tt_rpar });
            if (err) return {};
            vals.push_back(val);
            if (tokens[si].type == tt_com) {
                nextToken(si);
                continue;
            }
            if (tokens[si].type == tt_rpar) break;
        }
    }
    nextToken(si);
    auto t = nameToFunction.find(name);
    if (t == nameToFunction.end()) {
        logError("No function with name of " + name, tokens[i - 1]);
        err = true;
        return {};
    }
    vector<Function> funcs = t->second;
    Function* funcToCall = nullptr;
    for (int j = 0; j < funcs.size(); j++) {
        if (funcs[j].paramNames.size() == vals.size()) {
            bool match = true;
            for (int k = 0; k < vals.size(); k++) {
                if (funcs[j].paramTypes[k].name != vals[k].type.name) match = false;
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
            if (funcs[j].paramNames.size() == vals.size()) {
                bool match = true;
                for (int k = 0; k < vals.size(); k++) {
                    bool useable = funcs[j].paramTypes[k].name == vals[k].type.name;
                    useable = useable || (isNumType(funcs[j].paramTypes[k]) && isNumType(vals[k].type));
                    if (!useable) match = false;
                }
                if (match) {
                    if (funcToCall != nullptr) {
                        logError("Function call is ambiguous", tokens[i - 1]);
                        err = true;
                        return {};
                    }
                    funcToCall = &funcs[j];
                }
            }
        }

        if (funcToCall == nullptr) {
            logError("No function overload uses given arguments", tokens[i - 1]);
            err = true;
            return {};
        }

        for (int k = 0; k < vals.size(); k++) {
            if (funcToCall->paramTypes[k].name != vals[k].type.name) {
                implicitCastNumber(funcToCall->paramTypes[k], vals[k]);
            }
        }
    }

    vector<LLVMValueRef> llvmVals;
    for (int j = 0; j < vals.size(); j++) {
        llvmVals.push_back(vals[j].llvmVal);
    }

    i = si;
    Value val;
    if (funcToCall->returnType.name != "void") {
        val.llvmVal = LLVMBuildCall2(builder, funcToCall->llvmType, funcToCall->llvmValue, llvmVals.data(), llvmVals.size(), name.c_str());
    } else {
        val.llvmVal = LLVMBuildCall2(builder, funcToCall->llvmType, funcToCall->llvmValue, llvmVals.data(), llvmVals.size(), "");
    }
    val.type = funcToCall->returnType;
    return val;
}

Value parseValue(int& i, bool& err, Scope& scope) {
    int si = i;
    Value val;
    switch (tokens[si].type) {
        case tt_id: {
            string id = *tokens[i].data.str;
            if (tokens[si + 1].type == tt_lpar) {
                nextToken(si);
                val = callFunction(id, si, err, scope);
                if (err) return {};
                break;
            }
            Variable var = getVariableFromName(*tokens[si].data.str, scope, err);
            if (err) {
                logError("Variable doesn't exist", tokens[si]);
                err = true;
                return {};
            }
            nextToken(si);
            val = var.val;
            break;
        }
        case tt_float: {
            val.type = getTypeFromName("f64", err);
            assert(!err);
            val.llvmVal = LLVMConstReal(val.type.llvmType, tokens[si].data.dec);
            val.constant = true;
            nextToken(si);
            break;
        }
        case tt_int: {
            val.type = getTypeFromName("u64", err);
            assert(!err);
            val.llvmVal = LLVMConstInt(val.type.llvmType, tokens[si].data.uint, 0);
            val.constant = true;
            nextToken(si);
            break;
        }
        case tt_sub: {
            // TODO: negate value
        }
        case tt_mul: {
            // TODO: dereference
        }
        case tt_and: {
            // TODO: get ptr
        }
        default: {
            logError("Couldn't interpit this as a value", tokens[i]);
            err = true;
            return {};
            break;
        }
    }

    i = si;
    return val;
}


int getTokenPrio(Token& token, bool& err) {
    switch (token.type) {

        case tt_add: {
            return 3;
        }

        default: {
            err = true;
            return 0;
        }
    }
}

void castNumberForBiop(Value& lval, Value& rval) {
    int lwidth = 0;
    int rwidth = 0;
    if (isIntType(lval.type)) {
        lwidth = LLVMGetIntTypeWidth(lval.type.llvmType);
    } else {
        if (lval.type.name == "f16") lwidth = 16;
        if (lval.type.name == "f32") lwidth = 32;
        if (lval.type.name == "f64") lwidth = 64;
    }
    if (isIntType(rval.type)) {
        rwidth = LLVMGetIntTypeWidth(rval.type.llvmType);
    } else {
        if (rval.type.name == "f16") rwidth = 16;
        if (rval.type.name == "f32") rwidth = 32;
        if (rval.type.name == "f64") rwidth = 64;
    }

    if (isFloatType(lval.type) && !isFloatType(rval.type)) {
        if (isIntSigned(rval.type)) {
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildSIToFP(builder, rval.llvmVal, lval.type.llvmType, "intToFloat");
            return;
        } else {
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildUIToFP(builder, rval.llvmVal, lval.type.llvmType, "intToFloat");
            return;
        }
    } else if (!isFloatType(lval.type) && isFloatType(rval.type)) {
        if (isIntSigned(lval.type)) {
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildSIToFP(builder, lval.llvmVal, rval.type.llvmType, "intToFloat");
            return;
        } else {
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildUIToFP(builder, lval.llvmVal, rval.type.llvmType, "intToFloat");
            return;
        }
    }

    if (isFloatType(lval.type) && isFloatType(rval.type)) {
        if (lval.constant) {
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildFPCast(builder, lval.llvmVal, rval.type.llvmType, "floatTofloat");
            return;
        } else if (rval.constant) {
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildFPCast(builder, rval.llvmVal, lval.type.llvmType, "floatTofloat");
            return;
        } else {
            if (lwidth < rwidth) {
                rval.type = lval.type;
                rval.llvmVal = LLVMBuildFPCast(builder, rval.llvmVal, lval.type.llvmType, "floatTofloat");
            } else {
                lval.type = rval.type;
                lval.llvmVal = LLVMBuildFPCast(builder, lval.llvmVal, rval.type.llvmType, "floatTofloat");
            }
            return;
        }
    }



    if (lwidth > rwidth && lval.constant == rval.constant) {
        if (isIntSigned(lval.type) || isIntSigned(rval.type)) {
            lval.type.name[0] = 'i';
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildSExt(builder, rval.llvmVal, lval.type.llvmType, "intToint");
            return;
        } else {
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildZExt(builder, rval.llvmVal, lval.type.llvmType, "intToint");
            return;
        }
    } else if (lval.constant == rval.constant) {
        if (isIntSigned(lval.type) || isIntSigned(rval.type)) {
            rval.type.name[0] = 'i';
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildSExt(builder, lval.llvmVal, rval.type.llvmType, "intToint");
            return;
        } else {
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildZExt(builder, lval.llvmVal, rval.type.llvmType, "intToint");
            return;
        }
    }

    if (lval.constant) {
        if (lwidth > rwidth) {
            if (isIntSigned(lval.type)) {
                rval.type.name[0] = 'i';
            }
            lval.type = rval.type;
            lval.llvmVal = LLVMBuildTrunc(builder, lval.llvmVal, rval.type.llvmType, "intToint");
        } else {
            if (isIntSigned(lval.type)) {
                rval.type.name[0] = 'i';
                lval.llvmVal = LLVMBuildSExt(builder, lval.llvmVal, rval.type.llvmType, "intToint");
            } else {
                lval.llvmVal = LLVMBuildZExt(builder, lval.llvmVal, rval.type.llvmType, "intToint");
            }
            lval.type = rval.type;
            return;
        }
    } else {
        if (rwidth > lwidth) {
            if (isIntSigned(rval.type)) {
                lval.type.name[0] = 'i';
            }
            rval.type = lval.type;
            rval.llvmVal = LLVMBuildTrunc(builder, rval.llvmVal, lval.type.llvmType, "intToint");
        } else {
            if (isIntSigned(rval.type)) {
                lval.type.name[0] = 'i';
                rval.llvmVal = LLVMBuildSExt(builder, rval.llvmVal, lval.type.llvmType, "intToint");
            } else {
                rval.llvmVal = LLVMBuildZExt(builder, rval.llvmVal, lval.type.llvmType, "intToint");
            }
            rval.type = rval.type;
            return;
        }
    }

    return;
}


bool typeMatchOrImplicitCast(Type& type, Value& val) {
    if (typeMatch(type, val.type)) return true;
    if (isNumType(type) && isNumType(val.type)) {
        implicitCastNumber(type, val);
        return true;
    }
    return false;
}

Value addValues(Value& lval, Value& rval, bool& err) {
    //TODO: overload function
    if (isTypeRef(lval.type)) {
        lval = dereferenceValue(lval);
    }
    if (isTypeRef(rval.type)) {
        rval = dereferenceValue(rval);
    }
    if (isNumType(lval.type) && isNumType(rval.type)) {
        Value val;
        castNumberForBiop(lval, rval);
        val.type = lval.type;
        val.constant = lval.constant && rval.constant;

        if (isFloatType(lval.type)) {
            val.llvmVal = LLVMBuildFAdd(builder, lval.llvmVal, rval.llvmVal, "addFloats");
        } else {
            val.llvmVal = LLVMBuildAdd(builder, lval.llvmVal, rval.llvmVal, "addInts");
        }
        return val;
    }
    err = true;
    return {};
}

string to_string(TokenType type) {
    switch (type) {
        case tt_endl: {
            return "end line";
        }
        case tt_lbar: {
            return "left bracket";
        }
        default: {
            assert(false);
            return "(Lazy dev didn't add this tokens to_string)";
        }
    }
}

Value parseStatment(int& i, bool& err, Scope& scope, const vector<TokenType>& dels, int retPrio) {
    int si = i;
    Value lval = parseValue(si, err, scope);
    if (err) return {};
    while (true) {
        for (int j = 0; j < dels.size(); j++) {
            if (tokens[si].type == dels[j]) {
                i = si;
                return lval;
            }
        }
        int prio = getTokenPrio(tokens[si], err);
        TokenType op = tokens[si].type;
        if (err) {
            logError("Expected a binary opperator, or end of statment", tokens[si]);
            return {};
        }
        if (retPrio >= prio) {
            i = si;
            return lval;
        }
        nextToken(si);
        Value rval = parseStatment(si, err, scope, dels, prio);
        if (err) {
            return {};
        }
        switch (op) {
            case tt_add: {
                lval = addValues(lval, rval, err);
                if (err) {
                    logError("Can't add values of type " + lval.type.name + " with " + rval.type.name, tokens[i], true);
                }
                break;
            }
            default: {
                assert(false);
                abort();
            }
        }
    }
    i = si;
    return lval;
}

void implementScope(int& i, Function& func, Scope& scope) {
    assert(tokens[i].type == tt_lcur);
    Value v;
    nextToken(i);
    if (tokens[i].type != tt_endl) {
        logError("Nothing else should be on the line a scope is started", tokens[i]);
        while (true) {
            if (tokens[i].type == tt_endl) break;
            if (tokens[i].type == tt_rcur) return;
            nextToken(i);
        }
    }
    nextToken(i);

    while (true) {
        bool err = false;
        if (tokens[i].type == tt_rcur) break;
        Type type = getType(i, err, false);
        if (!err) {
            if (tokens[i].type != tt_id) {
                logError("Expected a variable name", tokens[i]);
                while (true) {
                    if (tokens[i].type == tt_endl) break;
                    if (tokens[i].type == tt_rcur) return;
                    nextToken(i);
                }
                nextToken(i);
                continue;
            }
            string varName = *tokens[i].data.str;
            nextToken(i);
            Variable var;
            var.name = varName;
            var.val.type = getTypeRef(type);
            var.val.llvmVal = LLVMBuildAlloca(builder, type.llvmType, varName.c_str());
            scope.nameToVariable[varName] = var;
            if (tokens[i].type == tt_endl) continue;
            switch (tokens[i].type) {
                case tt_eq: {
                    nextToken(i);
                    Value rval = parseStatment(i, err, scope, { tt_endl });
                    if (err) {
                        err = false;
                        while (true) {
                            if (tokens[i].type == tt_endl) break;
                            if (tokens[i].type == tt_rcur) return;
                            nextToken(i);
                        }
                        nextToken(i);
                        continue;
                    }
                    assert(tokens[i].type == tt_endl);
                    nextToken(i);
                    if (!typeMatchOrImplicitCast(getTypePointingTo(var.val.type, err), rval)) {
                        logError("Type of variable and right side do not match", tokens[i], true);
                        continue;
                    }
                    LLVMBuildStore(builder, rval.llvmVal, var.val.llvmVal);
                    continue;
                }
                default: {
                    logError("Expected an assignment of the variable or end line", tokens[i]);
                    while (true) {
                        if (tokens[i].type == tt_endl) break;
                        if (tokens[i].type == tt_rcur) return;
                        nextToken(i);
                    }
                    nextToken(i);
                    continue;
                }
            }

        } else {
            err = false;
            switch (tokens[i].type) {
                case tt_id: {
                    string name = *tokens[i].data.str;
                    Variable var = getVariableFromName(name, scope, err);
                    if (err) {
                        err = false;
                        nextToken(i);
                        Value lval = parseStatment(i, err, scope, { tt_eq, tt_endl });
                        if (err) {
                            while (true) {
                                if (tokens[i].type == tt_endl) break;
                                if (tokens[i].type == tt_rcur) return;
                                nextToken(i);
                            }
                            nextToken(i);
                            continue;
                        } else if (tokens[i].type == tt_eq) {
                            if (isTypePtr(lval.type)) { }
                            Value rval = parseStatment(i, err, scope, { tt_eq, tt_endl });
                        } else {  // tt_endl
                            continue;
                        }
                    } else {
                    }
                }
                default: {
                    logError("Line can't start with this token", tokens[i]);
                    while (true) {
                        if (tokens[i].type == tt_endl) break;
                        if (tokens[i].type == tt_rcur) return;
                        nextToken(i);
                    }
                    nextToken(i);
                    continue;
                }
            }
        }
    }
}

void implementFunction(Function& func) {
    int i = func.tokenPos;
    while (true) {
        nextToken(i);
        if (tokens[i].type == tt_lcur) break;
    }

    Scope scope;
    scope.parent = nullptr;
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func.llvmValue, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    for (int j = 0; j < func.paramNames.size(); j++) {
        Variable var;
        var.name = func.paramNames[j];
        var.val.type = getTypeRef(func.paramTypes[j]);
        var.val.llvmVal = LLVMBuildAlloca(builder, func.paramTypes[j].llvmType, func.paramNames[j].c_str());
        LLVMBuildStore(builder, LLVMGetParam(func.llvmValue, j), var.val.llvmVal);
        scope.nameToVariable[var.name] = var;
    }
    implementScope(i, func, scope);
}
LLVMValueRef createFormatString(LLVMModuleRef module, const char* str) {
    // Create a global constant for the format string
    LLVMValueRef formatStr = LLVMAddGlobal(module, LLVMArrayType(LLVMInt8Type(), strlen(str) + 1), "fmt");
    LLVMSetInitializer(formatStr, LLVMConstString(str, strlen(str) + 1, true));
    LLVMSetGlobalConstant(formatStr, true);
    LLVMSetLinkage(formatStr, LLVMPrivateLinkage);

    // Get pointer to the start of the string
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef indices[] = { zero, zero };
    return LLVMBuildInBoundsGEP2(builder, LLVMTypeOf(formatStr), formatStr, indices, 2, "fmtPtr");
}

void addPrint() {
    LLVMTypeRef printfArgTypes[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef printfType = LLVMFunctionType(LLVMInt32Type(), printfArgTypes, 1, true);
    LLVMValueRef printfFunc = LLVMAddFunction(llvmModule, "printf", printfType);

    LLVMTypeRef paramTypes[] = { LLVMInt64Type() };
    LLVMTypeRef printI64Type = LLVMFunctionType(LLVMVoidType(), paramTypes, 1, false);
    LLVMValueRef printI64Func = LLVMAddFunction(llvmModule, "print_i64", printI64Type);

    Function intPrint;
    intPrint.llvmType = printI64Type;
    intPrint.llvmValue = printI64Func;
    intPrint.name = "print";
    intPrint.paramNames = { "a" };
    bool err;
    intPrint.paramTypes = { getTypeFromName("i64", err) };
    intPrint.returnType = getTypeFromName("void", err);
    intPrint.tokenPos = 0;
    nameToFunction["print"].push_back(intPrint);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(printI64Func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef i64Value = LLVMGetParam(printI64Func, 0);
    LLVMValueRef formatStr = createFormatString(llvmModule, "%ld\n");
    LLVMValueRef printfArgs[] = { formatStr, i64Value };
    LLVMBuildCall2(builder, printfType, printfFunc, printfArgs, 2, "calltmp");
    LLVMBuildRetVoid(builder);
}

void addDefaults() {
    createType("void", LLVMVoidType());
    for (int i = 1; i < 256; i++) {
        createType("u" + to_string(i), LLVMIntType(i));
        createType("i" + to_string(i), LLVMIntType(i));
    }
    createType("f32", LLVMFloatType());
    createType("f64", LLVMDoubleType());
    createType("f16", LLVMHalfType());

    bool err;
    aliasType("float", getTypeFromName("f32", err));
    aliasType("int", getTypeFromName("i32", err));
    aliasType("uint", getTypeFromName("u32", err));
    aliasType("char", getTypeFromName("u8", err));
    addPrint();
}

void compileModule(const string& dir) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        cout << endl << "Directory does not exist or is not accessible" << endl << endl;
        return;
    }
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    dataLayout = LLVMGetModuleDataLayout(llvmModule);
    addDefaults();
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        files.push_back(path.string());
        textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
        getTokens(files.size() - 1);
    }
    findFunctionStarts();
    for (int i = 0; i < functionStarts.size(); i++) {
        prototypeFunction(functionStarts[i]);
    }
    for (auto keyVal : nameToFunction) {
        for (int i = 0; i < keyVal.second.size(); i++) {
            implementFunction(keyVal.second[i]);
        }
    }
}


void compile(const std::string& dir) {
    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
    compileModule(dir);
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
