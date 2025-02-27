#pragma once
#include "utils.h"
enum TokenType {
    tt_err,
    tt_int,
    tt_float,
    tt_str,
    tt_id,
    tt_add,
    tt_sub,
    tt_mul,
    tt_div,
    tt_eq,
    tt_eqeq,
    tt_endl,
    tt_lbar,
    tt_rbar,
    tt_lcur,
    tt_rcur,
    tt_lpar,
    tt_rpar,
    tt_car,
    tt_or,
    tt_bitor,
    tt_and,
    tt_bitand,
    tt_leeq,
    tt_greq,
    tt_gr,
    tt_neq,
    tt_for,
    tt_while,
    tt_break,
    tt_continue,
    tt_ex,
    tt_le,
    tt_ret,
    tt_com,
    tt_semi,
    tt_eof,
    tt_eot,
    tt_elips,
    tt_if,
    tt_else,
    tt_lshift,
    tt_rshift,
    tt_as,
    tt_to,
    tt_addeq,
    tt_subeq,
    tt_muleq,
    tt_struct,
    tt_diveq,
};

union TokenData {
    string* str;
    u64 uint;
    f64 dec;
};

struct Token {
    TokenType type;
    u8 file;
    u16 schar;
    u16 echar;
    u16 line;
    TokenData data;

    Token()
        : type(tt_int)
        , file(0)
        , schar(0)
        , echar(0)
        , line(0) {
        data.uint = 0;
    }

    ~Token() {
        cleanup();
    }

    // Copy constructor
    Token(const Token& other)
        : type(other.type)
        , file(other.file)
        , schar(other.schar)
        , echar(other.echar)
        , line(other.line) {
        if (type == tt_id || type == tt_str) {
            data.str = new string(*other.data.str);
        } else {
            data = other.data;
        }
    }

    // Copy assignment operator
    Token& operator=(const Token& other) {
        if (this == &other) return *this;

        cleanup();  // Clean up existing data

        type = other.type;
        file = other.file;
        schar = other.schar;
        echar = other.echar;
        line = other.line;

        if (type == tt_id || type == tt_str) {
            data.str = new string(*other.data.str);
        } else {
            data = other.data;
        }

        return *this;
    }

    // Move constructor
    Token(Token&& other) noexcept
        : type(other.type)
        , file(other.file)
        , schar(other.schar)
        , echar(other.echar)
        , line(other.line) {
        if (type == tt_id || type == tt_str) {
            data.str = other.data.str;
            other.data.str = nullptr;
        } else {
            data = other.data;
        }
        other.type = tt_err;
    }

    // Move assignment operator
    Token& operator=(Token&& other) noexcept {
        if (this == &other) return *this;

        cleanup();  // Clean up existing data

        type = other.type;
        file = other.file;
        schar = other.schar;
        echar = other.echar;
        line = other.line;

        if (type == tt_id || type == tt_str) {
            data.str = other.data.str;
            other.data.str = nullptr;
        } else {
            data = other.data;
        }
        other.type = tt_err;

        return *this;
    }

private:
    void cleanup() {
        if (type == tt_id || type == tt_str) {
            delete data.str;
            data.str = nullptr;
        }
    }
};

struct TokenPositon {
    u8 file;
    u16 line;
    u16 index;
};

class Tokens {
public:
    void addFileTokens(const vector<vector<string>>& textByFileByLine) {
        tokensByFileByLine.clear();
        tokensByFileByLine.resize(textByFileByLine.size());
        for (u16 file = 0; file < textByFileByLine.size(); file++) {
            vector<vector<Token>>& tokensByLine = tokensByFileByLine[file];
            tokensByLine.resize(textByFileByLine[file].size());
            for (u16 lineNum = 0; lineNum < textByFileByLine[file].size(); lineNum++) {
                vector<Token>& tokens = tokensByLine[lineNum];
                const string& line = textByFileByLine[file][lineNum];
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
                            token.echar = c;
                            string str = ss.str();
                            if (str == "return") {
                                token.type = tt_ret;
                                break;
                            }
                            if (str == "if") {
                                token.type = tt_if;
                                break;
                            }
                            if (str == "else") {
                                token.type = tt_else;
                                break;
                            }
                            if (str == "for") {
                                token.type = tt_for;
                                break;
                            }
                            if (str == "while") {
                                token.type = tt_while;
                                break;
                            }
                            if (str == "break") {
                                token.type = tt_break;
                                break;
                            }
                            if (str == "continue") {
                                token.type = tt_continue;
                                break;
                            }
                            if (str == "continue") {
                                token.type = tt_struct;
                                break;
                            }
                            if (str == "as") {
                                token.type = tt_as;
                                break;
                            }
                            if (str == "struct") {
                                token.type = tt_struct;
                                break;
                            }
                            if (str == "to") {
                                token.type = tt_to;
                                break;
                            }
                            if (str == "loop") {
                                // loop basiclly working like a macro for while true
                                Token token2;
                                token2.echar = token.echar;
                                token2.schar = token.schar;
                                token2.file = token.file;
                                token2.line = token.line;
                                token2.data.str = new string;
                                *token2.data.str = "while";
                                token2.type = tt_id;

                                token.type = tt_int;
                                token.data.uint = 1;
                                tokens.push_back(token2);
                                break;
                            }
                            if (str == "true") {
                                token.type == tt_int;
                                token.data.uint = 1;
                                break;
                            }
                            if (str == "false") {
                                token.type == tt_int;
                                token.data.uint = 0;
                                break;
                            }
                            // Add more keywords here
                            token.data.str = new string;
                            *token.data.str = str;
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
                                    case '\\': {
                                        if (c + 1 < line.size() && line[c + 1] == 'n') {
                                            c += 2;
                                            ss << '\n';
                                            break;
                                        }
                                        if (c + 1 < line.size() && line[c + 1] == '\\') {
                                            c += 2;
                                            ss << '\\';
                                            break;
                                        }
                                        if (c + 1 < line.size() && line[c + 1] == '0') {
                                            c += 2;
                                            ss << '\0';
                                            break;
                                        }
                                        if (c + 1 < line.size() && line[c + 1] == 't') {
                                            c += 2;
                                            ss << '\t';
                                            break;
                                        }
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
                            token.echar = c;
                            token.data.str = new string;
                            *token.data.str = ss.str();
                            token.type = tt_str;
                            break;
                        }
                        case '.': {
                            if (c + 1 < line.size() && line[c + 1] == '.') {
                                if (c + 2 < line.size() && line[c + 2] == '.') {
                                    c += 2;
                                    token.echar = c;
                                    token.type = tt_elips;
                                    break;
                                }
                            }
                        }
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
                                token.type = tt_or;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_bitor;
                            token.echar = c;
                            break;
                        }
                        case '&': {
                            if (c + 1 < line.size() && line[c + 1] == '&') {
                                c++;
                                token.type = tt_and;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_bitand;
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
                        case '!': {
                            if (c + 1 < line.size() && line[c + 1] == '=') {
                                c++;
                                token.type = tt_neq;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_ex;
                            token.echar = c;
                            break;
                        }
                        case '+': {
                            if (c + 1 < line.size() && line[c + 1] == '=') {
                                c++;
                                token.type = tt_addeq;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_add;
                            token.echar = c;
                            break;
                        }
                        case ';': {
                            token.type = tt_semi;
                            token.echar = c;
                            break;
                        }
                        case '-': {
                            if (c + 1 < line.size() && line[c + 1] == '=') {
                                c++;
                                token.type = tt_subeq;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_sub;
                            token.echar = c;
                            break;
                        }
                        case '*': {
                            if (c + 1 < line.size() && line[c + 1] == '=') {
                                c++;
                                token.type = tt_muleq;
                                token.echar = c;
                                break;
                            }
                            token.type = tt_mul;
                            token.echar = c;
                            break;
                        }
                        case '/': {
                            if (c + 1 < line.size() && line[c + 1] == '=') {
                                c++;
                                token.type = tt_diveq;
                                token.echar = c;
                                break;
                            }
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
                Token eol;
                eol.type = tt_endl;
                eol.schar = line.size();
                eol.echar = line.size();
                eol.line = lineNum;
                eol.file = file;
                tokensByLine[lineNum].push_back(eol);
            }
            Token eof;
            eof.type = tt_eof;
            eof.schar = UINT16_MAX;
            eof.echar = UINT16_MAX;
            eof.line = UINT16_MAX;
            eof.file = file;
            tokensByFileByLine[file].back().push_back({ eof });
        }
        Token eot;
        eot.type = tt_eot;
        eot.schar = UINT16_MAX;
        eot.echar = UINT16_MAX;
        eot.line = UINT16_MAX;
        eot.file = UINT8_MAX;
        tokensByFileByLine.push_back({ { eot } });
    }

    void nextToken() {
        pos.index++;
        if (pos.index >= tokensByFileByLine[pos.file][pos.line].size()) {
            pos.line++;
            pos.index = 0;
            if (pos.line >= tokensByFileByLine[pos.file].size()) {
                pos.file++;
                pos.line = 0;
            }
        }
    }

    void lastToken() {
        if (pos.index == 0) {
            if (pos.line == 0) {
                assert(pos.file != 0);
                pos.file--;
                pos.line = tokensByFileByLine[pos.file].size() - 1;
            } else {
                pos.line--;
                pos.index = tokensByFileByLine[pos.file][pos.line].size() - 1;
            }
        } else {
            pos.index--;
        }
    }

    Token getToken() {
        return tokensByFileByLine[pos.file][pos.line][pos.index];
    }



public:
    TokenPositon pos = { 0, 0, 0 };

private:
    vector<vector<vector<Token>>> tokensByFileByLine;

private:
};
