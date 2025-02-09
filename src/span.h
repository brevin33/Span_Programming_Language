#pragma once
#include <inttypes.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>

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

enum TokenType
{
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
};

union TokenData
{
    std::string* str;
    u64 uint;
    f64 dec;
};

struct Token
{
    u8 type;
    u8 file;
    u16 schar;
    u16 echar;
    u16 line;
    TokenData data;
    ~Token()
    {
        if (type == tt_id || type == tt_str) delete data.str;
    }
};

struct Module
{
    std::vector<std::string> files;
    std::vector<std::vector<std::string>> textByFileByLine;
    std::vector<std::vector<Token>> tokensByFile;
};
