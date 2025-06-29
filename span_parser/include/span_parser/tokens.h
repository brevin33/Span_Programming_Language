#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"


typedef enum _TokenType : u8 {
    tt_invalid = 0,
    tt_return,
    tt_if,
    tt_for,
    tt_struct,
    tt_enum,
    tt_union,
    tt_interface,
    tt_id,
    tt_dot,
    tt_number,
    tt_string_end,
    tt_string_continue,
    tt_rbrace,
    tt_lbrace,
    tt_comma,
    tt_colon,
    tt_semicolon,
    tt_add,
    tt_sub,
    tt_mul,
    tt_div,
    tt_mod,
    tt_eq,
    tt_neq,
    tt_lt,
    tt_gt,
    tt_le,
    tt_ge,
    tt_lparen,
    tt_rparen,
    tt_lbracket,
    tt_rbracket,
    tt_assign,
    tt_assign_add,
    tt_assign_sub,
    tt_assign_mul,
    tt_assign_div,
    tt_assign_mod,
    tt_assign_infer,
    tt_colon_colon,
    tt_negate,
    tt_uptr,
    tt_sptr,
    tt_char,
    tt_endl,
    tt_eof,
} OurTokenType;


typedef struct _Token {
    OurTokenType type;
    u8 tokenLength;
    u16 file;
    u32 tokenStart;
} Token;

Token* createTokens(Arena arena, char* fileContents, u64 fileIndex);

Token createToken(char* fileContent, u32* indexRef, u16 fileIndex, char* stringStack, u64* stringStackIndex);

char* ourTokenTypeToString(OurTokenType type);

char* tokenGetString(Token token, char* buffer);

char tokenGetTypeChar(Token token);

void tokenGetLineColumn(Token token, u64* outLine, u64* outColumnStart, u64* outColumnEnd);
