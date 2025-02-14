#include "span.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

LLVMContextRef context;
LLVMBuilderRef builder;
LLVMModuleRef llvmModule;
std::vector<std::string> files;
std::vector<std::vector<std::string>> textByFileByLine;
std::vector<Token> tokens;
std::vector<u64> functionStarts;
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
    cout << "Error: " << err << endl;
    cout << "In file " << files[token.file] << " On line " << token.line << endl;
    cout << endl;
    cout << textByFileByLine[token.file][token.line] << endl;
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
    cout << endl;
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

bool looksLikeType(int& i) {
    int si = i;
    if (tokens[si].type != tt_id) return false;
    if (++si >= tokens.size()) return false;
    // TODO: Type mod

    i = si;
    return true;
}

bool looksLikeFunction(int& i) {
    int si = i;
    if (!looksLikeType(si)) return false;
    if (tokens[si].type != tt_id) return false;
    if (++si >= tokens.size()) return false;
    if (tokens[si].type != tt_lpar) return false;
    while (true) {
        if (++si >= tokens.size()) return false;
        if (tokens[si].type == tt_rpar) break;
    }
    if (++si >= tokens.size()) return false;
    if (tokens[si].type != tt_lcur) return false;
    int curStack = 1;
    while (curStack != 0) {
        if (++si >= tokens.size()) return false;
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
                        if (++i >= tokens.size()) return;
                        if (tokens[i].type == tt_lcur) curStack--;
                        if (tokens[i].type == tt_rcur) curStack++;
                    }
                    if (++i >= tokens.size()) return;
                    if (tokens[i].type == tt_endl) break;
                    if (tokens[i].type == tt_eof) break;
                    logError("Right Brackets should be on there own line", tokens[i]);
                }
                if (++i >= tokens.size()) return;
            }
        }
    }
}

void compileModule(const string& dir) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        cout << endl << "Directory does not exist or is not accessible" << endl << endl;
        return;
    }
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        files.push_back(path.string());
        textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
        getTokens(files.size() - 1);
    }
    findFunctionStarts();
}


void compile(const std::string& dir) {
    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
    compileModule(dir);
    if (hadError) {
        cout << endl << "There was an error" << endl;
    }
}
