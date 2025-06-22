#pragma once

#include "nice_ints.h"
#include "arena.h"

typedef enum _TokenType : u8 {
    tt_error,
    tt_eop,  // end of project
    tt_eof,
    tt_endl,
    tt_id,
    tt_string,
    tt_int,
    tt_float,
    tt_char,
    tt_add,
    tt_sub,
    tt_mul,
    tt_div,
    tt_mod,
    tt_and,
    tt_or,
    tt_xor,
    tt_not,
    tt_eq,
    tt_neq,
    tt_lt,
    tt_gt,
    tt_leq,
    tt_geq,
    tt_assign,
    tt_add_assign,
    tt_sub_assign,
    tt_mul_assign,
    tt_div_assign,
    tt_mod_assign,
    tt_inc,
    tt_dec,
    tt_lparen,
    tt_rparen,
    tt_lbracket,
    tt_rbracket,
    tt_lbrace,
    tt_rbrace,
    tt_comma,
    tt_colon,
    tt_if,
    tt_else,
    tt_while,
    tt_for,
    tt_return,
    tt_break,
    tt_continue,
    tt_switch,
    tt_case,
    tt_default,
    tt_struct,
    tt_enum,
    tt_bool,
    tt_bit_and,
    tt_bit_or,
    tt_dot,
    tt_elips,
    tt_lshift,
    tt_rshift,
    tt_extern_c,
    tt_extern,
    tt_str_expr_start,
    tt_str_expr_end,
} OurTokenType;


typedef struct _Token {
    OurTokenType type;
    u32 file;
    u32 line;
    u16 charStart;
    u16 charEnd;
    char* str;
} Token;

bool assignLikeToken(OurTokenType type);

u64 getTokenInt(Token* token);

Token* loadTokensFromFile(char* fileContent, Arena* arena, u64 fileNumber);

char* tokenToString(Token* token, void* buffer, u64 bufferSize);
