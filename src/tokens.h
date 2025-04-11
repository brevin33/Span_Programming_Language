#pragma once
#include "utils.h"

enum TokenType : u8 {
    tt_error = 0,
    tt_identifier,
    tt_real,
    tt_integer,
    tt_string,
    tt_endl,
    tt_endf,
};


class Token {
public:
    TokenType type;
    u16 start;
    u16 end;
    u16 line;
    u8 file;
    union {
        u64 integer;
        double real;
        char* str = nullptr;
    };
    Token(TokenType type, u16 charStart, u16 charEnd, u16 line, u8 file)
        : type(type)
        , start(charStart)
        , end(charEnd)
        , line(line)
        , file(file)
        , real(0) {
    }
    Token(TokenType type, u16 charStart, u16 charEnd, u16 line, u8 file, string s)
        : type(type)
        , start(charStart)
        , end(charEnd)
        , line(line)
        , file(file) {
        assert(type == tt_string || type == tt_identifier);
        str = new char[s.length() + 1];
        strncpy(str, s.c_str(), s.size());
        str[s.length()] = '\0';
    }
    Token(TokenType type, u16 charStart, u16 charEnd, u16 line, u8 file, double r)
        : type(type)
        , start(charStart)
        , end(charEnd)
        , line(line)
        , file(file) {
        real = r;
    }
    Token(TokenType type, u16 charStart, u16 charEnd, u16 line, u8 file, u64 i)
        : type(type)
        , start(charStart)
        , end(charEnd)
        , line(line)
        , file(file) {
        integer = i;
    }

    string getString();
};

static std::string to_string(Token token) {
    switch (token.type) {
        case tt_error:
            return "tt_error";
        case tt_identifier:
            return "tt_identifier: " + token.getString();
        case tt_real:
            return "tt_real: " + to_string(token.real);
        case tt_integer:
            return "tt_integer: " + to_string(token.integer);
        case tt_string:
            return "tt_string: " + token.getString();
        case tt_endl:
            return "tt_endl";
        case tt_endf:
            return "tt_endf";
        default:
            return "unknown";
    }
}


class Tokens {
public:
    Tokens() {
    }

    void loadFile(string path);

    void print();

    Token getToken();

    ~Tokens() {
        for (auto& token : tokens) {
            if (token.type == tt_string || token.type == tt_identifier) {
                delete[] token.str;
            }
        }
    }

private:
    vector<Token> tokens;
};
