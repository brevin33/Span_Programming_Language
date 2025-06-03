#include "parser.h"
#include "parser/project.h"
#include <stdarg.h>
#include <stdio.h>
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

char errorUnderlineBuffer[2048];
char* getLineFromTokensWithUnderLine(Token* tokens, u64 numberOfTokens, Project* project) {
    while (tokens->type == tt_endl) {
        tokens++;
        numberOfTokens--;
    }
    if (numberOfTokens <= 0) {
        return NULL;  // No tokens to process
    }
    char* buffer = errorUnderlineBuffer;
    u64 bufferIndex = 0;
    char* charStartInLine = project->sourceFiles[tokens->file] + tokens->charStart;
    char* charEndInLine = project->sourceFiles[tokens->file] + tokens[numberOfTokens - 1].charEnd;
    char* lineStart = charStartInLine;
    // walk back to the start of the line
    while (lineStart > project->sourceFiles[tokens->file] && *(lineStart - 1) != '\n') {
        lineStart--;
    }
    char* lineEnd = charStartInLine;
    // walk forward to the end of the line
    while (*lineEnd != '\0' && *lineEnd != '\n') {
        lineEnd++;
    }
    if (*lineEnd == '\n') {
        lineEnd++;  // Move past the newline character
    }
    // add line to buffer
    memcpy(buffer + bufferIndex, lineStart, lineEnd - lineStart);
    bufferIndex += (lineEnd - lineStart);  // +1 for the newline character
    // add underline
    u64 lineLength = lineEnd - lineStart;
    u64 i = 0;
    bool inComment = false;
    while (i < lineLength) {
        if (inComment) {
            if (i + 1 < lineLength && lineStart[i] == '*' && lineStart[i + 1] == '/') {
                inComment = false;
                i++;
            }
            i++;
            continue;
        }
        i64 amountTillUnder = charStartInLine - lineStart - i;
        i64 amountPastUnder = charEndInLine - lineStart - i;
        bool underToken = amountTillUnder <= 0 && amountPastUnder >= 0;
        // only do it if under non space characters
        underToken = underToken && (lineStart[i] != ' ' && lineStart[i] != '\t' && lineStart[i] != '\0' && lineStart[i] != '\n' && lineStart[i] != '\r');
        if (lineStart[i] == '/' && (i + 1 < lineLength) && lineStart[i + 1] == '/') {
            break;
        }
        if (lineStart[i] == '/' && (i + 1 < lineLength) && lineStart[i + 1] == '*') {
            inComment = true;
            i++;
            continue;
        } else if (lineStart[i] == '\t') {
            buffer[bufferIndex++] = '\t';
        } else if (underToken) {
            buffer[bufferIndex++] = '^';
        } else {
            buffer[bufferIndex++] = ' ';
        }
        i++;
    }
    buffer[bufferIndex++] = '\n';  // Add newline after the underline
    bool isCharEndOnSameLine = (charEndInLine >= lineStart && charEndInLine <= lineEnd);
    // add line and then underline until at charEnd
    char* lastEndLine = lineEnd;  // Move past the newline character
    while (!isCharEndOnSameLine) {
        char* nextLineStart = lastEndLine;
        char* nextLineEnd = lastEndLine;
        // find the next newline
        while (*nextLineEnd != '\0' && *nextLineEnd != '\n') {
            nextLineEnd++;
        }
        if (*nextLineEnd == '\0') {
            break;
        }
        if (*nextLineEnd == '\n') {
            nextLineEnd++;  // Move past the newline character
        }
        // add the next line to the buffer
        memcpy(buffer + bufferIndex, nextLineStart, nextLineEnd - nextLineStart);
        bufferIndex += (nextLineEnd - nextLineStart);
        lastEndLine = nextLineEnd;
        lineLength = nextLineEnd - nextLineStart;
        // add underline for the next line
        int i = 0;
        while (i < lineLength) {
            if (inComment) {
                if (i + 1 < lineLength && lineStart[i] == '*' && lineStart[i + 1] == '/') {
                    inComment = false;
                    i++;
                }
                i++;
                continue;
            }
            i64 amountPastUnder = charEndInLine - nextLineStart - i;
            bool underToken = amountPastUnder >= 0;
            // only do it if under non space characters
            underToken = underToken && (nextLineStart[i] != ' ' && nextLineStart[i] != '\t' && nextLineStart[i] != '\0' && nextLineStart[i] != '\n' && nextLineStart[i] != '\r');

            if (nextLineStart[i] == '/' && (i + 1 < lineLength) && nextLineStart[i + 1] == '/') {
                break;
            }
            if (nextLineStart[i] == '/' && (i + 1 < lineLength) && nextLineStart[i + 1] == '*') {
                inComment = true;
                i++;
                continue;
            } else if (nextLineStart[i] == '\t') {
                buffer[bufferIndex++] = '\t';
            } else if (underToken) {
                buffer[bufferIndex++] = '^';
            } else {
                buffer[bufferIndex++] = ' ';
            }
            i++;
        }
        buffer[bufferIndex++] = '\n';  // Add newline after the underline
        isCharEndOnSameLine = (charEndInLine >= nextLineStart && charEndInLine <= nextLineEnd);
    }
    return buffer;
}

void logErrorToken(char* error, Project* project, Token* token, ...) {
    // just call logErrorTokens with a single token
    va_list args;
    va_start(args, token);
    logErrorTokens(error, project, token, 1, args);
    va_end(args);
}


void logErrorTokens(char* error, Project* project, Token* tokens, u64 numberOfTokens, ...) {
    // Use redvPrintf to print the error message in red
    va_list args;
    va_start(args, numberOfTokens);
    char* fileName = project->souceFileNames[tokens->file];
    redPrintf("Error in file %s on line ", fileName);
    int lastLine = -1;
    for (u64 i = 0; i < numberOfTokens; i++) {
        int line = tokens[i].line;
        if (line != lastLine) {
            if (lastLine != -1) {
                redPrintf(", ");
            }
            redPrintf("%d", line);
            lastLine = line;
        }
    }
    redPrintf(": ");
    redvPrintf(error, args);
    printf("\n");
    char* underlineStuff = getLineFromTokensWithUnderLine(tokens, numberOfTokens, project);
    if (underlineStuff != NULL) {
        printf("%s", underlineStuff);
    }
    printBar();
    va_end(args);
}
