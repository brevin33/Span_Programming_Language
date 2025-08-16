#pragma once

#include "default.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"

#ifdef _WIN32
    #define debugbreak() __debugbreak()
#else
    #define debugbreak()
#endif

#ifdef NDEBUG
    #define massert(condition, message)
#else
    #define massert(condition, message)                                                                                                                                                                                              \
        if (!(condition)) {                                                                                                                                                                                                          \
            makeRed();                                                                                                                                                                                                               \
            printf("Assertion failed: %s\n", message);                                                                                                                                                                               \
            resetColor();                                                                                                                                                                                                            \
            debugbreak();                                                                                                                                                                                                            \
            abort();                                                                                                                                                                                                                 \
        }
#endif



void logError(const char* message, ...);

void logErrorTokens(Token* tokens, u64 tokenCount, const char* message, ...);

void logErrorAst(SpanAst* ast, const char* message, ...);

void __mAssert(bool condition, const char* message);

void makeRed();

void resetColor();

void printBar();

void greenprintf(const char* message, ...);

void redprintf(const char* message, ...);
