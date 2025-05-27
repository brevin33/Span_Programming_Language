#pragma once
#include "nice_ints.h"
#include "arena.h"
#include <string>
#include <vector>

enum TokenType : u8 {
    tt_id,
    tt_int,
    tt_float,
    tt_string,
    tt_char,
    tt_comment,
    tt_eof,
    tt_lparen,
    tt_rparen,
    tt_lbrace,
    tt_rbrace,
    tt_lbracket,
    tt_rbracket,
    tt_semicolon,
    tt_colon,
    tt_comma,
    tt_plus,
    tt_minus,
    tt_multiply,
    tt_divide,
    tt_modulus,
    tt_equal,
    tt_assign,
    tt_not_equal,
    tt_less_than,
    tt_greater_than,
    tt_less_than_equal,
    tt_greater_than_equal,
    tt_and,
    tt_or,
    tt_xor,
    tt_bitwise_and,
    tt_bitwise_or,
    tt_bitwise_xor,
    tt_bitwise_not,
    tt_shift_left,
    tt_shift_right,
    tt_error,
    tt_return,
    tt_if,
    tt_else,
    tt_while,
    tt_for,
    tt_switch,
    tt_case,
    tt_sptr,
    tt_not,
    tt_endline,
    tt_endfile,
};

struct Token {
    TokenType type;
    u16 charStart;
    u16 charEnd;
    char* str;
};

struct Tokens {
    Tokens(std::string file);
    ~Tokens() {
    }
    Tokens(Tokens&&) noexcept = default;  // allow move
    Tokens& operator=(Tokens&&) noexcept = default;
    Token getToken();
    Token peekToken() const;
    Token peekToken(int amount) const;
    Token prevToken() const;
    Token prevToken(int amount) const;
    std::vector<std::vector<Token>> tokensByLine;
    u64 line;
    u64 index;
    Arena arena;
};

std::string to_string(const TokenType& type);
std::string to_string(const Token& token);
std::string to_string(const Tokens& tokens);
