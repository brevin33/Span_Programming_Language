#pragma once
#include <inttypes.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Support.h>
#include <optional>
#include <assert.h>

using namespace std;
namespace fs = std::filesystem;

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


string loadFileToString(const string& filePath);

vector<string> splitStringByNewline(const string& str);

string removeSpaces(const string& str);
