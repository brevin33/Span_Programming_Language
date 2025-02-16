#pragma once
#include <inttypes.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Support.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef double f64;
typedef float f32;

#define CASELETTER                                                                                                                                                                                                                   \
    case 'a':                                                                                                                                                                                                                        \
    case 'A':                                                                                                                                                                                                                        \
    case 'b':                                                                                                                                                                                                                        \
    case 'B':                                                                                                                                                                                                                        \
    case 'c':                                                                                                                                                                                                                        \
    case 'C':                                                                                                                                                                                                                        \
    case 'd':                                                                                                                                                                                                                        \
    case 'D':                                                                                                                                                                                                                        \
    case 'e':                                                                                                                                                                                                                        \
    case 'E':                                                                                                                                                                                                                        \
    case 'f':                                                                                                                                                                                                                        \
    case 'F':                                                                                                                                                                                                                        \
    case 'g':                                                                                                                                                                                                                        \
    case 'G':                                                                                                                                                                                                                        \
    case 'h':                                                                                                                                                                                                                        \
    case 'H':                                                                                                                                                                                                                        \
    case 'i':                                                                                                                                                                                                                        \
    case 'I':                                                                                                                                                                                                                        \
    case 'j':                                                                                                                                                                                                                        \
    case 'J':                                                                                                                                                                                                                        \
    case 'k':                                                                                                                                                                                                                        \
    case 'K':                                                                                                                                                                                                                        \
    case 'l':                                                                                                                                                                                                                        \
    case 'L':                                                                                                                                                                                                                        \
    case 'm':                                                                                                                                                                                                                        \
    case 'M':                                                                                                                                                                                                                        \
    case 'n':                                                                                                                                                                                                                        \
    case 'N':                                                                                                                                                                                                                        \
    case 'o':                                                                                                                                                                                                                        \
    case 'O':                                                                                                                                                                                                                        \
    case 'p':                                                                                                                                                                                                                        \
    case 'P':                                                                                                                                                                                                                        \
    case 'q':                                                                                                                                                                                                                        \
    case 'Q':                                                                                                                                                                                                                        \
    case 'r':                                                                                                                                                                                                                        \
    case 'R':                                                                                                                                                                                                                        \
    case 's':                                                                                                                                                                                                                        \
    case 'S':                                                                                                                                                                                                                        \
    case 't':                                                                                                                                                                                                                        \
    case 'T':                                                                                                                                                                                                                        \
    case 'u':                                                                                                                                                                                                                        \
    case 'U':                                                                                                                                                                                                                        \
    case 'v':                                                                                                                                                                                                                        \
    case 'V':                                                                                                                                                                                                                        \
    case 'w':                                                                                                                                                                                                                        \
    case 'W':                                                                                                                                                                                                                        \
    case 'x':                                                                                                                                                                                                                        \
    case 'X':                                                                                                                                                                                                                        \
    case 'y':                                                                                                                                                                                                                        \
    case 'Y':                                                                                                                                                                                                                        \
    case 'z':                                                                                                                                                                                                                        \
    case 'Z'

#define CASENUMBER                                                                                                                                                                                                                   \
    case '0':                                                                                                                                                                                                                        \
    case '1':                                                                                                                                                                                                                        \
    case '2':                                                                                                                                                                                                                        \
    case '3':                                                                                                                                                                                                                        \
    case '4':                                                                                                                                                                                                                        \
    case '5':                                                                                                                                                                                                                        \
    case '6':                                                                                                                                                                                                                        \
    case '7':                                                                                                                                                                                                                        \
    case '8':                                                                                                                                                                                                                        \
    case '9'

void compile(const std::string& dir);

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
    tt_oror,
    tt_and,
    tt_andand,
    tt_leeq,
    tt_greq,
    tt_gr,
    tt_le,
    tt_ret,
    tt_com,
    tt_eof,
};

union TokenData {
    std::string* str;
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
            data.str = new std::string(*other.data.str);
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
            data.str = new std::string(*other.data.str);
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

struct Type {
    std::string name;
    LLVMTypeRef llvmType;
    std::string anything;
};

struct Value {
    Type type;
    LLVMValueRef llvmVal;
    bool constant = false;
};

struct Variable {
    std::string name;
    Value val;
};

struct Scope {
    std::unique_ptr<Scope> parent;
    std::unordered_map<std::string, Variable> nameToVariable;
};

struct Function {
    std::string name;
    LLVMTypeRef llvmType;
    LLVMValueRef llvmValue;
    int tokenPos;
    Type returnType;
    std::vector<Type> paramTypes;
    std::vector<std::string> paramNames;
};
