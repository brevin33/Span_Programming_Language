#include "span.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

LLVMContextRef context;
LLVMBuilderRef builder;

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

void logError(const string& err, Token& token, Module& module) {
    cout << "Error: " << err << endl;
    cout << "In file " << module.files[token.file] << " On line " << token.line << endl;
    cout << endl;
    cout << module.textByFileByLine[token.file][token.line] << endl;
    for (int i = 0; i < token.schar; i++) {
        cout << " ";
    }
    for (int i = token.schar; i <= token.echar; i++) {
        cout << "^";
    }
    cout << endl;
    cout << "-------------------------------------" << endl;
}

void logError(const string& err, vector<Token>& tokens, Module& module) {
    cout << "Error: " << err << endl;
    cout << "In file " << module.files[tokens[0].file] << " On line " << tokens[0].line << endl;
    cout << endl;
    cout << module.textByFileByLine[tokens[0].file][tokens[0].line] << endl;
    for (int i = 0; i < tokens[0].schar; i++) {
        cout << " ";
    }
    for (int i = tokens[0].schar; i <= tokens.back().echar; i++) {
        cout << "^";
    }
    cout << endl;
    cout << endl;
    cout << "-------------------------------------" << endl;
}

void getTokens(u8 file, Module& module) {
    module.tokensByFileByLine.push_back({});
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine.back();
    for (u16 lineNum = 0; lineNum < module.textByFileByLine[file].size(); lineNum++) {
        string& line = module.textByFileByLine[file][lineNum];
        tokensByLine.push_back({});
        vector<Token>& tokens = tokensByLine.back();
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
    }
    tokensByLine.push_back({});
    Token eof;
    eof.type = tt_eof;
    eof.schar = UINT16_MAX;
    eof.echar = UINT16_MAX;
    eof.line = UINT16_MAX;
    eof.file = file;
    tokensByLine.back().push_back(eof);
}

bool nextTokenIndex(u16& line, u16& i, vector<vector<Token>>& tokensByLine) {
    if (i + 1 >= tokensByLine[line].size()) {
        if (line + 1 >= tokensByLine.size()) return true;
        i = 0;
        line++;
        while (line < tokensByLine.size() && tokensByLine[line].size() == 0)
            line++;
        return line >= tokensByLine.size();
    }
    i++;
    return false;
}

bool nextTokenLine(u16& line, u16& i, vector<vector<Token>>& tokensByLine) {
    if (i + 1 >= tokensByLine[line].size()) return true;
    i++;
    return false;
}

bool looksLikeType(u8& sfile, u16& sline, u16& si, Module& module) {
    u8 file = sfile;
    u16 i = si;
    u16 line = sline;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];

    if (tokensByLine[line][i].type != tt_id) return false;
    if (nextTokenIndex(line, i, tokensByLine)) return true;
    // TODO: add type modifiers

    sfile = file;
    si = i;
    sline = line;
    return true;
}

bool looksLikeFunction(u8& sfile, u16& sline, u16& si, Module& module) {
    u8 file = sfile;
    u16 i = si;
    u16 line = sline;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];

    if (!looksLikeType(file, line, i, module)) return false;

    if (tokensByLine[line][i].type != tt_id) return false;
    if (nextTokenIndex(line, i, tokensByLine)) return false;

    if (tokensByLine[line][i].type != tt_lpar) return false;
    if (nextTokenIndex(line, i, tokensByLine)) return false;

    while (tokensByLine[line][i].type != tt_rpar) {
        if (nextTokenIndex(line, i, tokensByLine)) return false;
    }
    if (nextTokenIndex(line, i, tokensByLine)) return false;

    if (tokensByLine[line][i].type != tt_lcur) return false;
    if (nextTokenIndex(line, i, tokensByLine)) return false;
    u32 curStack = 1;
    while (curStack != 0) {
        if (tokensByLine[line][i].type == tt_rcur) curStack--;
        if (tokensByLine[line][i].type == tt_lcur) curStack++;
        if (nextTokenIndex(line, i, tokensByLine)) return false;
    }

    sfile = file;
    si = i;
    sline = line;
    return true;
}

void findStarts(Module& module) {
    for (u8 file = 0; file < module.files.size(); file++) {
        vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];
        for (u16 line = 0; line < tokensByLine.size(); line++) {
            for (u16 i = 0; i < tokensByLine[line].size(); i++) {
                u8 fileStart = file;
                u16 lineStart = line;
                u16 iStart = i;
                if (looksLikeFunction(file, line, i, module)) module.functionStarts.push_back({ fileStart, lineStart, iStart });
                if (tokensByLine[line][i].type == tt_eof) {
                    continue;
                } else {
                    logError("Unknown Top Level Statments", tokensByLine[line], module);
                    // added to skip over non global scope in case of a function declaration being messed up
                    for (i; i < tokensByLine[line].size(); i++) {
                        if (tokensByLine[line][i].type != tt_lcur) continue;
                        u32 curStack = 1;
                        while (curStack != 0) {
                            if (tokensByLine[line][i].type == tt_rcur) curStack--;
                            if (tokensByLine[line][i].type == tt_lcur) curStack--;
                            if (nextTokenIndex(line, i, tokensByLine)) return;
                        }
                    }
                }
            }
        }
    }
}

Type createType(const string& name, LLVMTypeRef llvmType) {
    Type type;
    type.name = name;
    type.llvmType = llvmType;
    return type;
}


// this should just be a copy of getType without the err logging
Type tryGetType(u8& sfile, u16& sline, u16& si, bool& err, Module& module) {
    u8 file = sfile;
    u16 i = si;
    u16 line = sline;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];

    if (tokensByLine[line][i].type != tt_id) {
        err = true;
        return {};
    }
    string baseTypeName = *tokensByLine[line][i].data.str;
    if (module.nameToTypeId.count(baseTypeName) == 0) {
        err = true;
        return {};
    }
    if (nextTokenLine(line, i, tokensByLine)) {
        err = true;
        return {};
    }
    //TODO: add type mods

    string typeName = baseTypeName;

    auto t = module.nameToTypeId.find(typeName);
    Type type;
    if (t != module.nameToTypeId.end()) {
        type = module.types[t->second];
    } else {
        //TODO: create llvm type if doesn't exist
    }

    sfile = file;
    si = i;
    sline = line;
    return type;
}

Type getType(u8& sfile, u16& sline, u16& si, bool& err, Module& module) {
    u8 file = sfile;
    u16 i = si;
    u16 line = sline;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];

    if (tokensByLine[line][i].type != tt_id) {
        err = true;
        return {};
    }
    string baseTypeName = *tokensByLine[line][i].data.str;
    if (module.nameToTypeId.count(baseTypeName) == 0) {
        logError("Type does not exist", tokensByLine[line][i], module);
        err = true;
        return {};
    }
    if (nextTokenLine(line, i, tokensByLine)) {
        logError("Type doesn't make since as last token on this line", tokensByLine[line][i], module);
        err = true;
        return {};
    }
    //TODO: add type mods

    string typeName = baseTypeName;

    auto t = module.nameToTypeId.find(typeName);
    Type type;
    if (t != module.nameToTypeId.end()) {
        type = module.types[t->second];
    } else {
        //TODO: create llvm type if doesn't exist
    }

    sfile = file;
    si = i;
    sline = line;
    return type;
}

void prototypeFunction(TokenPosition& funcStart, Module& module) {
    u8 file = funcStart.file;
    u16 line = funcStart.line;
    u16 i = funcStart.i;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];
    bool err = false;

    Type retType = getType(file, line, i, err, module);
    if (err) return;

    string funcName = *tokensByLine[line][i].data.str;
    nextTokenIndex(line, i, tokensByLine);
    nextTokenIndex(line, i, tokensByLine);

    vector<Type> paramTypes;
    vector<string> paramNames;
    if (tokensByLine[line][i].type != tt_rpar) {
        while (true) {
            Type paramType = getType(file, line, i, err, module);
            if (err) return;

            if (tokensByLine[line][i].type != tt_id) {
                logError("Expected parameter name", tokensByLine[line][i], module);
                return;
            }
            string paramName = *tokensByLine[line][i].data.str;
            nextTokenIndex(line, i, tokensByLine);

            if (tokensByLine[line][i].type == tt_com) {
                nextTokenIndex(line, i, tokensByLine);
                continue;
            }
            if (tokensByLine[line][i].type == tt_rpar) {
                break;
            }
            logError("Expected , or )", tokensByLine[line][i], module);
            return;
        }
    }

    vector<LLVMTypeRef> llvmParamTypes;
    for (int j = 0; j < paramTypes.size(); j++) {
        llvmParamTypes.push_back(paramTypes[j].llvmType);
    }
    Function funciton;
    funciton.llvmType = LLVMFunctionType(retType.llvmType, llvmParamTypes.data(), llvmParamTypes.size(), 0);
    funciton.llvmVal = LLVMAddFunction(module.llvmModule, funcName.c_str(), funciton.llvmType);
    funciton.name = funcName;
    funciton.start = funcStart;
    funciton.paramNames = paramNames;
    funciton.paramTypes = paramTypes;
    funciton.returnType = retType;
    funciton.start = funcStart;
    return;
}

void implementFunction(Function& function, Module& module) {
    TokenPosition& funcStart = function.start;
    u8 file = funcStart.file;
    u16 line = funcStart.line;
    u16 i = funcStart.i;
    bool err = false;
    vector<vector<Token>>& tokensByLine = module.tokensByFileByLine[file];

    while (tokensByLine[line][i].type != tt_lcur) {
        nextTokenIndex(line, i, tokensByLine);
    }
    nextTokenIndex(line, i, tokensByLine);

    if (i != 0) {
        logError("No more tokens should be on this line", tokensByLine[line][i], module);
        line++;
    }

    // TODO: loop over all lines or somthing with this code
    Type type = tryGetType(file, line, i, err, module);
    if (!err) {
        // type
        if (tokensByLine[line][i].type != tt_id) {
            logError("Expected variable name", tokensByLine[line][i], module);
            // TODO: handel error
        }
        string variableName = *tokensByLine[line][i].data.str;
        if (nextTokenLine(line, i, tokensByLine)) {
            // TODO: define variable
        }
        switch (tokensByLine[line][i].type) {
        case tt_eq: {
            // TODO: handel assign variable
        }
        default: {
            logError("Didn't expect this after variable declaration", tokensByLine[line][i], module);
            // TODO: handel error
        }
        }
    } else {
        // not type
        // TODO
    }



    return;
}

LLVMTypeRef determineReturnType(LLVMTypeRef lhsType, LLVMTypeRef rhsType) {
    if (LLVMGetTypeKind(lhsType) == LLVMFloatTypeKind || LLVMGetTypeKind(rhsType) == LLVMFloatTypeKind) {
        if (LLVMGetTypeKind(lhsType) == LLVMFloatTypeKind && LLVMGetTypeKind(rhsType) == LLVMFloatTypeKind) {
            return LLVMFloatType();
        }
        return LLVMDoubleType();
    }

    if (LLVMGetTypeKind(lhsType) == LLVMIntegerTypeKind && LLVMGetTypeKind(rhsType) == LLVMIntegerTypeKind) {
        unsigned lhsWidth = LLVMGetIntTypeWidth(lhsType);
        unsigned rhsWidth = LLVMGetIntTypeWidth(rhsType);
        if (lhsWidth > rhsWidth) {
            return lhsType;
        } else if (rhsWidth > lhsWidth) {
            return rhsType;
        } else {
            return lhsType;
        }
    }
    return lhsType;
}

string llvmNumberTypeToString(LLVMTypeRef type) {
    switch (LLVMGetTypeKind(type)) {
    case LLVMFloatTypeKind:
        return "f32";
    case LLVMDoubleTypeKind:
        return "f64";
    case LLVMIntegerTypeKind: {
        unsigned width = LLVMGetIntTypeWidth(type);
        if (width == 8) return "i8";
        if (width == 16) return "i16";
        if (width == 32) return "i32";
        if (width == 64) return "i64";
        break;
    }
    default:
        return "unknown";
    }
    return "unknown";
}

Function createAddFunction(Module& module, LLVMBuilderRef builder, const string name, Type lhsType, Type rhsType) {
    // Define the function type: (lhsType, rhsType) -> type
    LLVMTypeRef paramTypes[] = { lhsType.llvmType, rhsType.llvmType };
    LLVMTypeRef returnType = determineReturnType(lhsType.llvmType, rhsType.llvmType);
    LLVMTypeRef funcType = LLVMFunctionType(returnType, paramTypes, 2, 0);

    // Create the function
    LLVMValueRef addFunction = LLVMAddFunction(module.llvmModule, name.c_str(), funcType);

    // Create entry block
    LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlock(addFunction, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBlock);

    // Retrieve the arguments
    LLVMValueRef lhs = LLVMGetParam(addFunction, 0);
    LLVMValueRef rhs = LLVMGetParam(addFunction, 1);

    // Handle casting if one operand is integer and the other is float
    if (LLVMGetTypeKind(lhsType.llvmType) == LLVMIntegerTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMFloatTypeKind) {
        lhs = LLVMBuildSIToFP(builder, lhs, LLVMFloatType(), "lhs_casted_to_float");
    } else if (LLVMGetTypeKind(lhsType.llvmType) == LLVMFloatTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMIntegerTypeKind) {
        rhs = LLVMBuildSIToFP(builder, rhs, LLVMFloatType(), "rhs_casted_to_float");
    }

    if (LLVMGetTypeKind(lhsType.llvmType) == LLVMIntegerTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMDoubleTypeKind) {
        lhs = LLVMBuildSIToFP(builder, lhs, LLVMDoubleType(), "lhs_casted_to_double");
    } else if (LLVMGetTypeKind(lhsType.llvmType) == LLVMDoubleTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMIntegerTypeKind) {
        rhs = LLVMBuildSIToFP(builder, rhs, LLVMDoubleType(), "rhs_casted_to_double");
    }

    if (LLVMGetTypeKind(lhsType.llvmType) == LLVMFloatTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMDoubleTypeKind) {
        lhs = LLVMBuildFPExt(builder, lhs, LLVMDoubleType(), "lhs_casted_to_double");
    } else if (LLVMGetTypeKind(lhsType.llvmType) == LLVMDoubleTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMFloatTypeKind) {
        rhs = LLVMBuildFPExt(builder, rhs, LLVMDoubleType(), "rhs_casted_to_double");
    }


    // Handle casting if both operands are integers but of different sizes
    if (LLVMGetTypeKind(lhsType.llvmType) == LLVMIntegerTypeKind && LLVMGetTypeKind(rhsType.llvmType) == LLVMIntegerTypeKind) {
        unsigned lhsWidth = LLVMGetIntTypeWidth(lhsType.llvmType);
        unsigned rhsWidth = LLVMGetIntTypeWidth(rhsType.llvmType);
        if (lhsWidth < rhsWidth) {
            if (rhsType.name[0] == 'u') {
                lhs = LLVMBuildZExt(builder, lhs, rhsType.llvmType, "lhs_promoted");
            } else {
                lhs = LLVMBuildSExt(builder, lhs, rhsType.llvmType, "lhs_promoted");
            }
        } else if (rhsWidth < lhsWidth) {
            if (rhsType.name[0] == 'u') {
                rhs = LLVMBuildZExt(builder, rhs, lhsType.llvmType, "lhs_promoted");
            } else {
                rhs = LLVMBuildSExt(builder, rhs, lhsType.llvmType, "lhs_promoted");
            }
        }
    }

    // Build the add or fadd instruction based on types
    LLVMValueRef result;
    if (LLVMGetTypeKind(lhsType.llvmType) == LLVMFloatTypeKind || LLVMGetTypeKind(rhsType.llvmType) == LLVMFloatTypeKind || LLVMGetTypeKind(lhsType.llvmType) == LLVMDoubleTypeKind
        || LLVMGetTypeKind(rhsType.llvmType) == LLVMDoubleTypeKind) {
        result = LLVMBuildFAdd(builder, lhs, rhs, "sum");
    } else {
        result = LLVMBuildAdd(builder, lhs, rhs, "sum");
    }

    // Return the result
    LLVMBuildRet(builder, result);

    Function func;
    func.llvmType = funcType;
    func.llvmVal = addFunction;
    func.name = name;
    func.start = {};
    Type retType;
    retType.llvmType = returnType;
    retType.name = llvmNumberTypeToString(returnType);
    func.returnType = retType;
    func.paramNames = { "lhs", "rhs" };
    vector<Type> pTypes;
    pTypes.resize(2);
    pTypes[0].name = llvmNumberTypeToString(paramTypes[0]);
    pTypes[1].name = llvmNumberTypeToString(paramTypes[1]);
    pTypes[0].llvmType = paramTypes[0];
    pTypes[1].llvmType = paramTypes[1];
    func.paramTypes = pTypes;

    return func;
}

void setupGenerics(Module& module) {
    module.nameToTypeId["i64"] = module.types.size();
    module.types.push_back(createType("i64", LLVMInt64Type()));

    module.nameToTypeId["i32"] = module.types.size();
    module.types.push_back(createType("i32", LLVMInt32Type()));

    module.nameToTypeId["i16"] = module.types.size();
    module.types.push_back(createType("i16", LLVMInt16Type()));

    module.nameToTypeId["i8"] = module.types.size();
    module.types.push_back(createType("i8", LLVMInt8Type()));

    module.nameToTypeId["u64"] = module.types.size();
    module.types.push_back(createType("u64", LLVMInt64Type()));

    module.nameToTypeId["u32"] = module.types.size();
    module.types.push_back(createType("u32", LLVMInt32Type()));

    module.nameToTypeId["u16"] = module.types.size();
    module.types.push_back(createType("u16", LLVMInt16Type()));

    module.nameToTypeId["u8"] = module.types.size();
    module.types.push_back(createType("u8", LLVMInt8Type()));

    module.nameToTypeId["f32"] = module.types.size();
    module.types.push_back(createType("f32", LLVMFloatType()));

    module.nameToTypeId["f64"] = module.types.size();
    module.types.push_back(createType("f64", LLVMDoubleType()));

    //for (u32 i = 0; i < module.types.size(); i++) {
    //    for (u32 j = 0; j < module.types.size(); j++) {
    //        Type type1 = module.types[i];
    //        Type type2 = module.types[j];
    //        Function addFunc = createAddFunction(module, builder, "add-" + type1.name + type2.name, type1, type2);
    //        module.nameToFunctionId[addFunc.name] = module.functions.size();
    //        module.functions.push_back(addFunc);
    //    }
    //}

    module.nameToTypeId["void"] = module.types.size();
    module.types.push_back(createType("void", LLVMVoidType()));
}

void compileModule(const string& dir) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        cout << endl << "Directory does not exist or is not accessible" << endl << endl;
        return;
    }
    Module module;
    module.llvmModule = LLVMModuleCreateWithName(dir.c_str());
    setupGenerics(module);
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        module.files.push_back(path.string());
        module.textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
        getTokens(module.files.size() - 1, module);
    }

    findStarts(module);

    for (u64 i = 0; i < module.functionStarts.size(); i++) {
        prototypeFunction(module.functionStarts[i], module);
    }

    for (u64 i = 0; i < module.functions.size(); i++) {
        implementFunction(module.functions[i], module);
    }
}


void compile(const std::string& dir) {
    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
    compileModule(dir);
}
