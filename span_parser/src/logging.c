#include "span_parser.h"
#include <stdarg.h>

void makeRed() {
    printf("\x1b[31m");
}

void resetColor() {
    printf("\x1b[0m");
}

void printBar() {
    printf("+------------------------------------------------------------------------------+\n");
}

void logError(const char* message, ...) {
    makeRed();
    printf("Error: ");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    printf("\n");
    resetColor();
    printBar();
}

void logErrorAst(SpanAst* ast, const char* message, ...) {
    va_list args;
    va_start(args, message);
    logErrorTokens(ast->token, ast->tokenLength, message, args);
    va_end(args);
}

void logErrorTokens(Token* tokens, u64 tokenCount, const char* message, ...) {
    makeRed();
    u64 lines[128];
    u64 tokenLines[128];
    u64 lineCount = 0;
    u64 columnStarts[128];
    u64 columnEnds[128];
    tokenCount = tokenCount > 128 ? 128 : tokenCount;
    for (u64 i = 0; i < tokenCount; i++) {
        u64 line;
        u64 columnStart;
        u64 columnEnd;
        tokenGetLineColumn(tokens[i], &line, &columnStart, &columnEnd);
        columnStarts[i] = columnStart;
        columnEnds[i] = columnEnd;
        tokenLines[i] = line;
        bool found = false;
        for (u64 j = 0; j < lineCount; j++) {
            if (lines[j] == line) {
                found = true;
                break;
            }
        }
        if (!found) {
            lines[lineCount] = line;
            lineCount++;
        }
    }
    char linesStr[256];
    for (u64 i = 0; i < lineCount; i++) {
        sprintf(linesStr, "%s,%llu", linesStr, lines[i]);
    }

    SpanProject* project = context.activeProject;
    SpanFile* file = &project->files[tokens[0].file];
    char* filename = file->fileName;
    if (lineCount == 1) {
        printf("Error on line %llu in file %s: ", lines[0], filename);
    } else {
        printf("Error on lines %s in file %s: ", linesStr, filename);
    }
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    printf("\n");
    resetColor();


    // print lines and underline the tokens
    for (u64 i = 0; i < lineCount; i++) {
        printf("%06llu| ", lines[i]);
        char lineBuffer[BUFFER_SIZE] = { 0 };
        u64 lineBufferCount = 0;
        memset(lineBuffer, 0, sizeof(lineBuffer));
        char* lineStart = file->fileLineStarts[lines[i] - 1];
        while (*lineStart != '\n') {
            lineBuffer[lineBufferCount++] = *lineStart;
            lineStart++;
        }
        lineBuffer[lineBufferCount] = '\0';
        printf(lineBuffer, "%06llu| %s\n", lines[i], lineBuffer);

        printf("\n");

        // underline the tokens
        printf("%06llu| ", lines[i]);
        lineStart = file->fileLineStarts[lines[i] - 1];
        u64 lineStartCount = 0;
        while (lineStart[lineStartCount] != '\n') {
            bool charIsToken = false;
            for (u64 j = 0; j < tokenCount; j++) {
                u64 tokenLine = tokenLines[j];
                if (tokenLine != lines[i]) {
                    continue;
                }
                u64 tokenColumnStart = columnStarts[j];
                u64 tokenColumnEnd = columnEnds[j];
                charIsToken = tokenColumnStart <= lineStartCount && tokenColumnEnd > lineStartCount;
                if (charIsToken) {
                    break;
                }
            }
            if (charIsToken) {
                printf("^");
            } else if (lineStart[lineStartCount] == '\t') {
                printf("\t");
            } else if (lineStart[lineStartCount] == '\r') {
            } else {
                printf(" ");
            }
            lineStartCount++;
        }

        bool charAtEndOfLine = false;
        for (u64 j = 0; j < tokenCount; j++) {
            u64 tokenLine = tokenLines[j];
            if (tokenLine != lines[i]) {
                continue;
            }
            if (tokens[j].type == tt_eof) {
                charAtEndOfLine = true;
                break;
            } else if (tokens[j].type == tt_end_statement) {
                charAtEndOfLine = true;
                break;
            }
        }
        if (charAtEndOfLine) {
            printf("^");
        }
    }


    printf("\n");

    printBar();
}
