#include "parser.h"
#include "parser/project.h"
#include "parser/sourceCode.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void printBar() {
    printf("--------------------------------------------------\n");
}

void redvPrintf(const char* format, va_list args) {
    printf("\033[31m");
    vprintf(format, args);
    printf("\033[0m");
}

void redPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[31m");
    vprintf(format, args);
    printf("\033[0m");
    va_end(args);
}

void greenvPrintf(const char* format, va_list args) {
    printf("\033[32m");
    vprintf(format, args);
    printf("\033[0m");
}

void greenPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[32m");
    vprintf(format, args);
    printf("\033[0m");
    va_end(args);
}

void yellowvPrintf(const char* format, va_list args) {
    printf("\033[33m");
    vprintf(format, args);
    printf("\033[0m");
}

void yellowPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[33m");
    vprintf(format, args);
    printf("\033[0m");
    va_end(args);
}

void bluevPrintf(const char* format, va_list args) {
    printf("\033[34m");
    vprintf(format, args);
    printf("\033[0m");
}

void bluePrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[34m");
    vprintf(format, args);
    printf("\033[0m");
    va_end(args);
}

void logError(char* error, ...) {
    // ue v redvPrintf to print the error message in red
    va_list args;
    va_start(args, error);
    redPrintf("Error: ");
    redvPrintf(error, args);
    printf("\n");
    printBar();
    va_end(args);
}

typedef struct __range {
    u64 start;
    u64 end;
} _range;

void printLineFromSourceCode(sourceCodeId sourceCodeId, u64 lineNum) {
    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
    if (lineNum > sourceCode->lineCount) {
        return;
    }
    char* line = sourceCode->lines[lineNum - 1];
    u64 lineLength = sourceCode->lineLengths[lineNum - 1];
    char buffer[2048];
    memcpy(buffer, line, lineLength);
    buffer[lineLength] = '\0';
    printf("%06llu | ", lineNum);
    printf("%s\n", buffer);
}

void underLineLineWithTokens(Token* tokens, u64 tokenCount, u64 lineNum, sourceCodeId sourceCodeId) {
    // print padding from line printing
    printf("%06llu | ", lineNum);

    SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
    if (lineNum > sourceCode->lineCount) {
        return;
    }
    char* line = sourceCode->lines[lineNum - 1];
    u64 lineLength = sourceCode->lineLengths[lineNum - 1];

    for (u64 i = 0; i < lineLength; i++) {
        if (line[i] == '\t') {
            printf("\t");
        } else if (line[i] == '\r') {
            //printf("\r");
        } else if (line[i] == ' ') {
            printf(" ");
        } else {
            bool anythingUnderChar = false;
            for (u64 j = 0; j < tokenCount; j++) {
                if (tokens[j].line != lineNum) {
                    continue;
                }
                if (tokens[j].file != sourceCodeId) {
                    continue;
                }
                if (tokens[j].charEnd < i) {
                    continue;
                }
                if (tokens[j].charStart > i) {
                    continue;
                }
                anythingUnderChar = true;
                break;
            }
            if (anythingUnderChar) {
                printf("^");
            } else {
                printf(" ");
            }
        }
    }

    // if we have and endl or eof on this put a ^ at the very end of the line
    for (u64 i = 0; i < tokenCount; i++) {
        if (tokens[i].line != lineNum) {
            continue;
        }
        if (tokens[i].file != sourceCodeId) {
            continue;
        }
        if (tokens[i].type == tt_endl || tokens[i].type == tt_eof) {
            printf("^");
            break;
        }
    }

    printf("\n");
}

int compareU64(const void* a, const void* b) {
    u64 aVal = *(const u64*)a;
    u64 bVal = *(const u64*)b;
    if (aVal < bVal) {
        return -1;
    } else if (aVal > bVal) {
        return 1;
    } else {
        return 0;
    }
}


void logErrorTokens(Token* token, u64 tokenCount, char* error, ...) {
    va_list args;
    va_start(args, error);
    redPrintf("Error: ");
    redvPrintf(error, args);
    printf("\n");
    va_end(args);
    if (tokenCount == 0) {
        return;
    }

    sourceCodeId fileToDo[32];
    u64 fileToDoCount = 0;
    for (u64 i = 0; i < tokenCount; i++) {
        bool doneFile = false;
        for (u64 j = 0; j < fileToDoCount; j++) {
            if (token[i].file == fileToDo[j]) {
                doneFile = true;
                break;
            }
        }
        if (doneFile) {
            continue;
        }
        if (fileToDoCount >= 32) {
            break;
        }
        fileToDo[fileToDoCount] = token[i].file;
        fileToDoCount++;
    }

    for (u64 i = 0; i < fileToDoCount; i++) {
        sourceCodeId currentFile = fileToDo[i];
        u64 linesToDo[128];
        u64 linesToDoCount = 0;
        for (u64 j = 0; j < tokenCount; j++) {
            bool doneLine = false;
            u64 line = token[j].line;
            for (u64 k = 0; k < linesToDoCount; k++) {
                if (token[j].line == linesToDo[k]) {
                    doneLine = true;
                    break;
                }
            }
            // if we've already done this line, skip it
            if (doneLine) {
                continue;
            }
            if (linesToDoCount >= 128) {
                break;
            }
            linesToDo[linesToDoCount] = token[j].line;
            linesToDoCount++;
        }

        // sort lines so we do them in order
        qsort(linesToDo, linesToDoCount, sizeof(u64), compareU64);

        for (u64 j = 0; j < linesToDoCount; j++) {
            u64 line = linesToDo[j];
            printLineFromSourceCode(currentFile, line);
            underLineLineWithTokens(token, tokenCount, line, currentFile);
        }
    }
    printBar();
}


void assertCrash(bool condition) {
    if (!condition) {
        crash();
    }
}

void crash() {
    printBar();
    redPrintf("go yell at this trash developer for having a bug in the program\n\n");
}
