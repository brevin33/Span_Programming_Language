#include "span.h"
#include <filesystem>
#include <fstream>
#include <assert.h>

using namespace std;
namespace fs = std::filesystem;

LLVMContextRef context;
LLVMBuilderRef builder;
LLVMModuleRef llvmModule;
std::vector<std::string> files;
std::vector<std::vector<std::string>> textByFileByLine;
std::vector<Token> tokens;
std::vector<u64> functionStarts;
unordered_map<string, Type> nameToType;
unordered_map<string, vector<Function>> nameToFunction;
bool hadError = false;

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

void logError(const string& err, Token& token, bool wholeLine = false) {
    hadError = true;
    std::cout << "\033[31m";
    cout << "Error: " << err << endl;
    std::cout << "\033[0m";
    cout << textByFileByLine[token.file][token.line] << endl;
    std::cout << "\033[31m";
    if (wholeLine) {
        for (int i = 0; i <= textByFileByLine[token.file][token.line].size(); i++) {
            cout << "^";
        }
    } else {
        for (int i = 0; i < token.schar; i++) {
            cout << " ";
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
        eof.schar = UINT16_MAX;
        eof.echar = UINT16_MAX;
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

bool typeMatch(Type type1, Type type2) {
    return type1.name == type2.name;
}

bool typeMatchOrImplicitCast(Type type, Value val) {
    if (typeMatch(type, val.type)) return true;
    // TODO: Implicit casting
    return false;
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

Type getTypePtr(Type type) {
    type.name = type.name + '*';
    type.llvmType = LLVMPointerType(type.llvmType, 0);
    return type;
}

Type getTypePointingTo(Type type, bool& err) {
    if (type.name.back() != '*') {
        err = true;
        return {};
    }
    type.name.resize(type.name.size() - 1);
    return getTypeFromName(type.name, err);
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
    func.llvmValue = LLVMAddFunction(llvmModule, funcName.c_str(), func.llvmType);
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
    Variable v = {};
    return v;
}

Value parseValue(int& i, bool& err, Scope& scope) {
    int si = i;
    Value val;
    switch (tokens[si].type) {
        case tt_id: {
            Variable var = getVariableFromName(*tokens[si].data.str, scope, err);
            if (err) {
                logError("Variable doesn't exist", tokens[si]);
                return {};
            }
            nextToken(si);
            if (tokens[si].type == tt_lpar) {
                // TODO: function call
                break;
            }
            Type type = getTypePointingTo(var.val.type, err);
            assert(!err);
            val.llvmVal = LLVMBuildLoad2(builder, type.llvmType, var.val.llvmVal, var.name.c_str());
            val.type = type;
            break;
        }
        case tt_float: {
            val.type = getTypeFromName("f64", err);
            assert(!err);
            val.llvmVal = LLVMConstReal(val.type.llvmType, tokens[si].data.dec);
            nextToken(si);
            break;
        }
        case tt_int: {
            val.type = getTypeFromName("u64", err);
            assert(!err);
            val.llvmVal = LLVMConstInt(val.type.llvmType, tokens[si].data.uint, 0);
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

Value addValues(Value& lval, Value& rval) {
    // TODO
    return lval;
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

Value parseStatment(int& i, bool& err, Scope& scope, TokenType del, int retPrio = INT_MIN) {
    int si = i;
    Value lval = parseValue(si, err, scope);
    if (err) return {};
    while (true) {
        if (tokens[si].type == del) {
            i = si;
            return lval;
        }
        int prio = getTokenPrio(tokens[si], err);
        TokenType op = tokens[si].type;
        if (err) {
            logError("Expected a binary opperator, or " + to_string(del), tokens[si]);
            return {};
        }
        if (retPrio >= prio) {
            i = si;
            return lval;
        }
        nextToken(si);
        Value rval = parseStatment(si, err, scope, del, prio);
        if (err) {
            return {};
        }
        switch (op) {
            case tt_add: {
                lval = addValues(lval, rval);
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
            var.val.type = getTypePtr(type);
            var.val.llvmVal = LLVMBuildAlloca(builder, type.llvmType, varName.c_str());
            scope.nameToVariable[varName] = var;
            if (tokens[i].type == tt_endl) continue;
            switch (tokens[i].type) {
                case tt_eq: {
                    nextToken(i);
                    Value rval = parseStatment(i, err, scope, tt_endl);
                    if (err) {
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
            switch (tokens[i].type) {
                case tt_id: {
                    //TODO:
                }
                default: {
                    logError("Line cann't start with this token", tokens[i]);
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
        var.name = func.paramNames[i];
        var.val.type = getTypePtr(func.paramTypes[i]);
        var.val.llvmVal = LLVMBuildAlloca(builder, func.paramTypes[j].llvmType, func.paramNames[j].c_str());
        LLVMBuildStore(builder, LLVMGetParam(func.llvmValue, j), var.val.llvmVal);
        scope.nameToVariable[var.name] = var;
    }
    implementScope(i, func, scope);
}

void addDefaults() {
    createType("void", LLVMVoidType());
    createType("int", LLVMIntType(32));
    createType("uint", LLVMIntType(32));
    createType("float", LLVMFloatType());
    for (int i = 1; i < 256; i++) {
        createType("u" + to_string(i), LLVMIntType(i));
        createType("i" + to_string(i), LLVMIntType(i));
    }
    createType("f32", LLVMDoubleType());
    createType("f64", LLVMDoubleType());
    createType("f16", LLVMHalfType());
}

void compileModule(const string& dir) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        cout << endl << "Directory does not exist or is not accessible" << endl << endl;
        return;
    }
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
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
