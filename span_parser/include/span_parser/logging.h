#pragma once
#include "default.h"
#include "span_parser/tokens.h"
#include "span_parser/ast.h"

#ifdef NDEBUG
    #define massert(condition, message)
#else
    #define massert(condition, message) __mAssert(condition, message)
#endif

#ifdef _WIN32
    #define debugbreak() __debugbreak()
#else
    #define debugbreak()
#endif


void logError(const char* message, ...);

void logErrorTokens(Token* tokens, u64 tokenCount, const char* message, ...);

void logErrorAst(SpanAst* ast, const char* message, ...);

void __mAssert(bool condition, const char* message);

void makeRed();

void resetColor();

void printBar();
