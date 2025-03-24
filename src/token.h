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
    tt_in,
    tt_neq,
    tt_for,
    tt_while,
    tt_break,
    tt_continue,
    tt_ex,
    tt_le,
    tt_ret,
    tt_case,
    tt_com,
    tt_semi,
    tt_eof,
    tt_default,
    tt_eot,
    tt_elips,
    tt_if,
    tt_else,
    tt_lshift,
    tt_rshift,
    tt_as,
    tt_to,
    tt_dot,
    tt_addeq,
    tt_subeq,
    tt_muleq,
    tt_struct,
    tt_elipseq,
    tt_diveq,
    tt_enum,
    tt_switch,
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

    string& getStr() {
        // Todo: look at template to replace if equal to template
        return *this->data.str;
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

int getTokens(const vector<vector<string>>& textByFileByLine, int startFile);
