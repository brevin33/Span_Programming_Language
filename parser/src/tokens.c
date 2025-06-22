#include "parser/tokens.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CASE_LETTER                                                                                                                                                                                                                  \
    case 'a':                                                                                                                                                                                                                        \
    case 'b':                                                                                                                                                                                                                        \
    case 'c':                                                                                                                                                                                                                        \
    case 'd':                                                                                                                                                                                                                        \
    case 'e':                                                                                                                                                                                                                        \
    case 'f':                                                                                                                                                                                                                        \
    case 'g':                                                                                                                                                                                                                        \
    case 'h':                                                                                                                                                                                                                        \
    case 'i':                                                                                                                                                                                                                        \
    case 'j':                                                                                                                                                                                                                        \
    case 'k':                                                                                                                                                                                                                        \
    case 'l':                                                                                                                                                                                                                        \
    case 'm':                                                                                                                                                                                                                        \
    case 'n':                                                                                                                                                                                                                        \
    case 'o':                                                                                                                                                                                                                        \
    case 'p':                                                                                                                                                                                                                        \
    case 'q':                                                                                                                                                                                                                        \
    case 'r':                                                                                                                                                                                                                        \
    case 's':                                                                                                                                                                                                                        \
    case 't':                                                                                                                                                                                                                        \
    case 'u':                                                                                                                                                                                                                        \
    case 'v':                                                                                                                                                                                                                        \
    case 'w':                                                                                                                                                                                                                        \
    case 'x':                                                                                                                                                                                                                        \
    case 'y':                                                                                                                                                                                                                        \
    case 'z':                                                                                                                                                                                                                        \
    case 'A':                                                                                                                                                                                                                        \
    case 'B':                                                                                                                                                                                                                        \
    case 'C':                                                                                                                                                                                                                        \
    case 'D':                                                                                                                                                                                                                        \
    case 'E':                                                                                                                                                                                                                        \
    case 'F':                                                                                                                                                                                                                        \
    case 'G':                                                                                                                                                                                                                        \
    case 'H':                                                                                                                                                                                                                        \
    case 'I':                                                                                                                                                                                                                        \
    case 'J':                                                                                                                                                                                                                        \
    case 'K':                                                                                                                                                                                                                        \
    case 'L':                                                                                                                                                                                                                        \
    case 'M':                                                                                                                                                                                                                        \
    case 'N':                                                                                                                                                                                                                        \
    case 'O':                                                                                                                                                                                                                        \
    case 'P':                                                                                                                                                                                                                        \
    case 'Q':                                                                                                                                                                                                                        \
    case 'R':                                                                                                                                                                                                                        \
    case 'S':                                                                                                                                                                                                                        \
    case 'T':                                                                                                                                                                                                                        \
    case 'U':                                                                                                                                                                                                                        \
    case 'V':                                                                                                                                                                                                                        \
    case 'W':                                                                                                                                                                                                                        \
    case 'X':                                                                                                                                                                                                                        \
    case 'Y':                                                                                                                                                                                                                        \
    case 'Z'

#define CASE_NUMBER                                                                                                                                                                                                                  \
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


char* tokenToString(Token* token, void* buffer, u64 bufferSize) {
    if (token->type == tt_id || token->type == tt_int || token->type == tt_float || token->type == tt_bool) {
        sprintf_s((char*)buffer, bufferSize, "%s", token->str);
        return buffer;
    }
    if (token->type == tt_char) {
        sprintf_s((char*)buffer, bufferSize, "'%s'", token->str);
        return buffer;
    }
    if (token->type == tt_string) {
        sprintf_s((char*)buffer, bufferSize, "\"%s\"", token->str);
        return buffer;
    }
    switch (token->type) {
        case tt_add:
            sprintf_s((char*)buffer, bufferSize, "add");
            return buffer;
        case tt_sub:
            sprintf_s((char*)buffer, bufferSize, "sub");
            return buffer;
        case tt_mul:
            sprintf_s((char*)buffer, bufferSize, "mul");
            return buffer;
        case tt_div:
            sprintf_s((char*)buffer, bufferSize, "div");
            return buffer;
        case tt_mod:
            sprintf_s((char*)buffer, bufferSize, "mod");
            return buffer;
        case tt_and:
            sprintf_s((char*)buffer, bufferSize, "and");
            return buffer;
        case tt_or:
            sprintf_s((char*)buffer, bufferSize, "or");
            return buffer;
        case tt_xor:
            sprintf_s((char*)buffer, bufferSize, "xor");
            return buffer;
        case tt_not:
            sprintf_s((char*)buffer, bufferSize, "not");
            return buffer;
        case tt_eq:
            sprintf_s((char*)buffer, bufferSize, "eq");
            return buffer;
        case tt_neq:
            sprintf_s((char*)buffer, bufferSize, "neq");
            return buffer;
        case tt_lt:
            sprintf_s((char*)buffer, bufferSize, "lt");
            return buffer;
        case tt_gt:
            sprintf_s((char*)buffer, bufferSize, "gt");
            return buffer;
        case tt_leq:
            sprintf_s((char*)buffer, bufferSize, "leq");
            return buffer;
        case tt_geq:
            sprintf_s((char*)buffer, bufferSize, "geq");
            return buffer;
        case tt_assign:
            sprintf_s((char*)buffer, bufferSize, "assign");
            return buffer;
        case tt_add_assign:
            sprintf_s((char*)buffer, bufferSize, "add_assign");
            return buffer;
        case tt_sub_assign:
            sprintf_s((char*)buffer, bufferSize, "sub_assign");
            return buffer;
        case tt_mul_assign:
            sprintf_s((char*)buffer, bufferSize, "mul_assign");
            return buffer;
        case tt_div_assign:
            sprintf_s((char*)buffer, bufferSize, "div_assign");
            return buffer;
        case tt_mod_assign:
            sprintf_s((char*)buffer, bufferSize, "mod_assign");
            return buffer;
        case tt_inc:
            sprintf_s((char*)buffer, bufferSize, "inc");
            return buffer;
        case tt_dec:
            sprintf_s((char*)buffer, bufferSize, "dec");
            return buffer;
        case tt_lparen:
            sprintf_s((char*)buffer, bufferSize, "lparen");
            return buffer;
        case tt_rparen:
            sprintf_s((char*)buffer, bufferSize, "rparen");
            return buffer;
        case tt_lbracket:
            sprintf_s((char*)buffer, bufferSize, "lbracket");
            return buffer;
        case tt_rbracket:
            sprintf_s((char*)buffer, bufferSize, "rbracket");
            return buffer;
        case tt_lbrace:
            sprintf_s((char*)buffer, bufferSize, "lbrace");
            return buffer;
        case tt_rbrace:
            sprintf_s((char*)buffer, bufferSize, "rbrace");
            return buffer;
        case tt_comma:
            sprintf_s((char*)buffer, bufferSize, "comma");
            return buffer;
        case tt_semi:
            sprintf_s((char*)buffer, bufferSize, "semi");
            return buffer;
        case tt_colon:
            sprintf_s((char*)buffer, bufferSize, "colon");
            return buffer;
        case tt_if:
            sprintf_s((char*)buffer, bufferSize, "if");
            return buffer;
        case tt_else:
            sprintf_s((char*)buffer, bufferSize, "else");
            return buffer;
        case tt_while:
            sprintf_s((char*)buffer, bufferSize, "while");
            return buffer;
        case tt_for:
            sprintf_s((char*)buffer, bufferSize, "for");
            return buffer;
        case tt_return:
            sprintf_s((char*)buffer, bufferSize, "return");
            return buffer;
        case tt_break:
            sprintf_s((char*)buffer, bufferSize, "break");
            return buffer;
        case tt_continue:
            sprintf_s((char*)buffer, bufferSize, "continue");
            return buffer;
        case tt_switch:
            sprintf_s((char*)buffer, bufferSize, "switch");
            return buffer;
        case tt_case:
            sprintf_s((char*)buffer, bufferSize, "case");
            return buffer;
        case tt_default:
            sprintf_s((char*)buffer, bufferSize, "default");
            return buffer;
        case tt_struct:
            sprintf_s((char*)buffer, bufferSize, "struct");
            return buffer;
        case tt_enum:
            sprintf_s((char*)buffer, bufferSize, "enum");
            return buffer;
        case tt_bool:
            sprintf_s((char*)buffer, bufferSize, "bool");
            return buffer;
        case tt_bit_and:
            sprintf_s((char*)buffer, bufferSize, "bit_and");
            return buffer;
        case tt_bit_or:
            sprintf_s((char*)buffer, bufferSize, "bit_or");
            return buffer;
        case tt_endl:
            sprintf_s((char*)buffer, bufferSize, "endl");
            return buffer;
        case tt_eof:
            sprintf_s((char*)buffer, bufferSize, "eof");
            return buffer;
        case tt_id:
            sprintf_s((char*)buffer, bufferSize, "id");
            return buffer;
        case tt_string:
            sprintf_s((char*)buffer, bufferSize, "string");
            return buffer;
        case tt_int:
            sprintf_s((char*)buffer, bufferSize, "int");
            return buffer;
        case tt_float:
            sprintf_s((char*)buffer, bufferSize, "float");
            return buffer;
        case tt_char:
            sprintf_s((char*)buffer, bufferSize, "char");
            return buffer;
        case tt_dot:
            sprintf_s((char*)buffer, bufferSize, "dot");
            return buffer;
        case tt_elips:
            sprintf_s((char*)buffer, bufferSize, "elips");
            return buffer;
        case tt_str_expr_start:
            sprintf_s((char*)buffer, bufferSize, "str_expr_start");
            return buffer;
        case tt_str_expr_end:
            sprintf_s((char*)buffer, bufferSize, "str_expr_end");
            return buffer;
        default:
            sprintf_s((char*)buffer, bufferSize, "unknown_token");
            return buffer;
    }
    return buffer;
}

u64 getTokenInt(Token* token) {
    assert(token->type == tt_int);
    u64 value = 0;
    char* str = token->str;
    while (*str != '\0') {
        u8 digit = *str - '0';
        if (value > (UINT64_MAX - digit) / 10) {
            logError("Integer token too large for u64: %s", token->str);
            return 0;
        }
        value = value * 10 + digit;
        str++;
    }
    return value;
}

u64 escapeStringBufferSize = 2048;
char* escapeStringBuffer = NULL;
//uses the char* input as the buffer to write the escaped string to
char* handelStringEscapes(char* str) {
    if (escapeStringBuffer == NULL) {
        escapeStringBuffer = malloc(escapeStringBufferSize);
    }
    char* buffer = escapeStringBuffer;
    u64 size = 0;
    for (char c = *str; c != '\0'; c = *(++str)) {
        size++;
        if (size >= escapeStringBufferSize) {
            escapeStringBufferSize *= 2;
            escapeStringBuffer = realloc(escapeStringBuffer, escapeStringBufferSize);
        }
        if (c == '\\') {
            str++;
            switch (*str) {
                case 'n':
                    *buffer++ = '\n';
                    break;
                case 'r':
                    *buffer++ = '\r';
                    break;
                case 't':
                    *buffer++ = '\t';
                    break;
                case '\\':
                    *buffer++ = '\\';
                    break;
                case '"':
                    *buffer++ = '"';
                    break;
                case '\'':
                    *buffer++ = '\'';
                    break;
                case '0':
                    *buffer++ = '\0';
                    break;
                case '{':
                    *buffer++ = '{';
                    break;
                case 'v':
                    *buffer++ = '\v';
                    break;
                case 'f':
                    *buffer++ = '\f';
                    break;
                case 'a':
                    *buffer++ = '\a';
                    break;
                case '?':
                    *buffer++ = '\?';
                    break;
                default:
                    *buffer++ = *str;
                    break;
            }
        }
    }
    memcpy(str, buffer, size);
    str[size] = '\0';
    return str;
}

// this function is a bit of a mess, but it works
void addToken(char* fileContent, u64 fileNumber, u64* iRef, Token* tokens, u64* tokenCountRef, u64* tokenCapacityRef, u64* lineNumberRef, u64* charPosRef, Arena* arena) {
    u64 lineNumber = *lineNumberRef;
    u64 i = *iRef;
    u64 tokenCount = *tokenCountRef;
    u64 tokenCapacity = *tokenCapacityRef;
    u64 iOfLastNewLine = *charPosRef;
    u64 iOfLastOveride = UINT64_MAX;

    while (true) {
        if (fileContent[i] == '\r' || fileContent[i] == ' ' || fileContent[i] == '\t') {
            i++;
            continue;
        }
        if (fileContent[i] == '/' && fileContent[i + 1] == '/') {
            // single line comment
            while (fileContent[i] != '\n') {
                iOfLastNewLine = i;
                i++;
            }
            continue;
        }
        if (fileContent[i] == '/' && fileContent[i + 1] == '*') {
            // multi-line comment
            u64 startLine = lineNumber;
            i += 2;  // Skip the '/*'
            while (fileContent[i] != '\0' && !(fileContent[i] == '*' && fileContent[i + 1] == '/')) {
                if (fileContent[i] == '\n') {
                    iOfLastNewLine = i;
                    lineNumber++;
                }
                i++;
            }
            if (fileContent[i] == '\0') {
                char* errorComment;
                logError("Unterminated multi-line comment starting at line %u", startLine);
                break;
            }
            i += 2;  // Skip the '*/'
            continue;
        }

        int number = fileContent[i];

        if (tokenCount >= tokenCapacity) {
            tokens = arenaRealloc(arena, tokens, sizeof(Token) * tokenCapacity, sizeof(Token) * tokenCapacity * 2);
            tokenCapacity *= 2;
        }
        u64 charStart = i - iOfLastNewLine - 1;
        switch (fileContent[i]) {
            case '_':
            CASE_LETTER: {
                // Identifier or keyword
                u64 start = i;
                while (fileContent[i] == '_' || (fileContent[i] >= 'a' && fileContent[i] <= 'z') || (fileContent[i] >= 'A' && fileContent[i] <= 'Z') || (fileContent[i] >= '0' && fileContent[i] <= '9')) {
                    i++;
                }
                u64 length = i - start;
                char* identifier = (char*)arenaAlloc(arena, length + 1);
                memcpy(identifier, &fileContent[start], length);
                identifier[length] = '\0';
                OurTokenType type = tt_id;  // Default type
                if (strcmp(identifier, "if") == 0) type = tt_if;
                else if (strcmp(identifier, "else") == 0)
                    type = tt_else;
                else if (strcmp(identifier, "while") == 0)
                    type = tt_while;
                else if (strcmp(identifier, "for") == 0)
                    type = tt_for;
                else if (strcmp(identifier, "return") == 0)
                    type = tt_return;
                else if (strcmp(identifier, "break") == 0)
                    type = tt_break;
                else if (strcmp(identifier, "continue") == 0)
                    type = tt_continue;
                else if (strcmp(identifier, "switch") == 0)
                    type = tt_switch;
                else if (strcmp(identifier, "case") == 0)
                    type = tt_case;
                else if (strcmp(identifier, "default") == 0)
                    type = tt_default;
                else if (strcmp(identifier, "struct") == 0)
                    type = tt_struct;
                else if (strcmp(identifier, "enum") == 0)
                    type = tt_enum;
                else if (strcmp(identifier, "true") == 0 || strcmp(identifier, "false") == 0)
                    type = tt_bool;
                else if (strcmp(identifier, "extern") == 0)
                    type = tt_extern;
                else if (strcmp(identifier, "extern_c") == 0)
                    type = tt_extern_c;
                tokens[tokenCount].type = type;
                tokens[tokenCount].str = identifier;
                break;
            }
            case '"': {
                // kinda bad but it works
                u64 start = i;
                bool isFormated = false;
                if (fileContent[i + 1] == '"' && fileContent[i + 2] == '"') {
                    // multi line string
                    i += 3;  // Skip the opening triple quotes
                    while (fileContent[i] != '\0' && !(fileContent[i] == '"' && fileContent[i + 1] == '"' && fileContent[i + 2] == '"')) {
                        if (fileContent[i] == '\n') {
                            iOfLastNewLine = i;
                            lineNumber++;
                        }
                        if (fileContent[i] == '\\') {
                            if (fileContent[i + 1] == '"') {  // Handle escaped quotes
                                i++;
                            }
                        }
                        if (fileContent[i] == '{') {
                            u64 length;
                            if (!isFormated) {
                                length = i - start - 3;
                            } else {
                                length = i - start - 1;
                            }
                            char* stringContent = (char*)arenaAlloc(arena, length + 1);
                            if (isFormated) {
                                memcpy(stringContent, &fileContent[start + 1], length);
                            } else {
                                memcpy(stringContent, &fileContent[start + 3], length);
                            }
                            stringContent[length] = '\0';
                            handelStringEscapes(stringContent);
                            u64 charEnd = i - iOfLastNewLine;
                            tokens[tokenCount++] = (Token) { tt_string, fileNumber, lineNumber, charStart, charEnd, stringContent };
                            // add the str expr start token
                            tokens[tokenCount++] = (Token) { tt_str_expr_start, fileNumber, lineNumber, charStart, charEnd + 1, "{" };
                            i++;
                            while (tokens[tokenCount - 1].type != tt_rbrace) {
                                addToken(fileContent, fileNumber, &i, tokens, &tokenCount, &tokenCapacity, &lineNumber, &iOfLastNewLine, arena);
                            }
                            tokens[tokenCount - 1].type = tt_str_expr_end;
                            isFormated = true;
                            i--;
                            start = i;
                        }
                        i++;
                    }
                    if (fileContent[i] == '\0') {
                        logError("Unterminated multi-line string starting at line %u", lineNumber);
                        break;
                    }
                    i += 3;  // Skip the closing triple quotes
                    if (!isFormated) {
                        u64 length = i - start - 6;  // Exclude the triple quotes
                        char* stringContent = (char*)arenaAlloc(arena, length + 1);
                        memcpy(stringContent, &fileContent[start + 3], length);
                        stringContent[length] = '\0';
                        handelStringEscapes(stringContent);
                        tokens[tokenCount].type = tt_string;
                        tokens[tokenCount].str = stringContent;
                        break;
                    } else {
                        start++;
                        u64 length = i - start - 3;
                        char* stringContent = (char*)arenaAlloc(arena, length + 1);
                        memcpy(stringContent, &fileContent[start], length);
                        stringContent[length] = '\0';
                        handelStringEscapes(stringContent);
                        tokens[tokenCount].type = tt_string;
                        tokens[tokenCount].str = stringContent;
                        break;
                    }
                }
                // Single line string
                i++;
                while (fileContent[i] != '\0' && fileContent[i] != '\n' && fileContent[i] != '"') {
                    if (fileContent[i] == '\\') {
                        if (fileContent[i + 1] == '"') {  // Handle escaped quotes
                            i++;
                        }
                    }
                    if (fileContent[i] == '{') {
                        // formated expresstion in string
                        // finish the string
                        u64 length = i - start - 1;
                        char* stringContent = (char*)arenaAlloc(arena, length + 1);
                        memcpy(stringContent, &fileContent[start + 1], length);
                        stringContent[length] = '\0';
                        handelStringEscapes(stringContent);
                        u64 charEnd = i - iOfLastNewLine;
                        tokens[tokenCount++] = (Token) { tt_string, fileNumber, lineNumber, charStart, charEnd, stringContent };
                        // add the str expr start token
                        tokens[tokenCount++] = (Token) { tt_str_expr_start, fileNumber, lineNumber, charStart, charEnd + 1, "{" };
                        i++;
                        while (tokens[tokenCount - 1].type != tt_rbrace) {
                            addToken(fileContent, fileNumber, &i, tokens, &tokenCount, &tokenCapacity, &lineNumber, &iOfLastNewLine, arena);
                        }
                        tokens[tokenCount - 1].type = tt_str_expr_end;
                        isFormated = true;
                        i--;
                        start = i;
                    }
                    i++;
                }
                if (fileContent[i] == '\0' || fileContent[i] == '\n') {
                    if (fileContent[i] == '\n') {
                        iOfLastNewLine = i;
                        lineNumber++;
                    }
                    logError("Unterminated string starting at line %u", lineNumber);
                    break;
                }
                i++;
                if (!isFormated) {
                    u64 length = i - start - 2;
                    char* stringContent = (char*)arenaAlloc(arena, length + 1);
                    memcpy(stringContent, &fileContent[start + 1], length);
                    stringContent[length] = '\0';
                    handelStringEscapes(stringContent);
                    tokens[tokenCount].type = tt_string;
                    tokens[tokenCount].str = stringContent;
                    break;
                } else {
                    start++;
                    u64 length = i - start - 1;
                    char* stringContent = (char*)arenaAlloc(arena, length + 1);
                    memcpy(stringContent, &fileContent[start], length);
                    stringContent[length] = '\0';
                    handelStringEscapes(stringContent);
                    tokens[tokenCount].type = tt_string;
                    tokens[tokenCount].str = stringContent;
                    break;
                }
            }
            CASE_NUMBER: {
                // Integer or float
                u64 start = i;
                bool isFloat = false;
                while (fileContent[i] >= '0' && fileContent[i] <= '9') {
                    i++;
                }
                if (fileContent[i] == '.') {
                    isFloat = true;
                    i++;
                    while (fileContent[i] >= '0' && fileContent[i] <= '9') {
                        i++;
                    }
                }
                u64 length = i - start;
                char* numberStr = (char*)arenaAlloc(arena, length + 1);
                memcpy(numberStr, &fileContent[start], length);
                numberStr[length] = '\0';
                OurTokenType type = isFloat ? tt_float : tt_int;
                tokens[tokenCount].type = type;
                tokens[tokenCount].str = numberStr;
                break;
            }
            case '\'': {
                // Character literal
                u64 start = i;
                i++;  // Skip the opening quote
                if (fileContent[i] == '\\') {
                    i += 2;  // Skip the escape sequence
                } else {
                    i++;  // Skip the character
                }
                if (fileContent[i] != '\'') {
                    logError("Unterminated character literal starting at line %u", lineNumber);
                    break;
                }
                i++;  // Skip the closing quote
                u64 length = i - start;
                char* charStr = (char*)arenaAlloc(arena, length + 1);
                memcpy(charStr, &fileContent[start], length);
                charStr[length] = '\0';
                tokens[tokenCount].type = tt_char;
                tokens[tokenCount].str = charStr;
                break;
            }
            case '+': {
                if (fileContent[i + 1] == '+') {
                    tokens[tokenCount].type = tt_inc;
                    tokens[tokenCount].str = "++";
                    i += 2;
                } else if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_add_assign;
                    tokens[tokenCount].str = "+=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_add;
                    tokens[tokenCount].str = "+";
                    i++;
                }
                break;
            }
            case '-': {
                if (fileContent[i + 1] == '-') {
                    tokens[tokenCount].type = tt_dec;
                    tokens[tokenCount].str = "--";
                    i += 2;
                } else if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_sub_assign;
                    tokens[tokenCount].str = "-=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_sub;
                    tokens[tokenCount].str = "-";
                    i++;
                }
                break;
            }
            case '*': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_mul_assign;
                    tokens[tokenCount].str = "*=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_mul;
                    tokens[tokenCount].str = "*";
                    i++;
                }
                break;
            }
            case '/': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_div_assign;
                    tokens[tokenCount].str = "/=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_div;
                    tokens[tokenCount].str = "/";
                    i++;
                }
                break;
            }
            case '%': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_mod_assign;
                    tokens[tokenCount].str = "%=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_mod;
                    tokens[tokenCount].str = "%";
                    i++;
                }
                break;
            }
            case '&': {
                if (fileContent[i + 1] == '&') {
                    tokens[tokenCount].type = tt_and;
                    tokens[tokenCount].str = "&&";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_bit_and;
                    tokens[tokenCount].str = "&";
                    i++;
                }
                break;
            }
            case '|': {
                if (fileContent[i + 1] == '|') {
                    tokens[tokenCount].type = tt_or;
                    tokens[tokenCount].str = "||";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_bit_or;
                    tokens[tokenCount].str = "|";
                    i++;
                }
                break;
            }
            case '^': {
                tokens[tokenCount].type = tt_xor;
                tokens[tokenCount].str = "^";
                i++;
                break;
            }
            case '!': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_neq;
                    tokens[tokenCount].str = "!=";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_not;
                    tokens[tokenCount].str = "!";
                    i++;
                }
                break;
            }
            case '=': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_eq;
                    tokens[tokenCount].str = "==";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_assign;
                    tokens[tokenCount].str = "=";
                    i++;
                }
                break;
            }
            case '<': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_leq;
                    tokens[tokenCount].str = "<=";
                    i += 2;
                } else if (fileContent[i + 1] == '<') {
                    tokens[tokenCount].type = tt_lshift;
                    tokens[tokenCount].str = "<<";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_lt;
                    tokens[tokenCount].str = "<";
                    i++;
                }
                break;
            }
            case '>': {
                if (fileContent[i + 1] == '=') {
                    tokens[tokenCount].type = tt_geq;
                    tokens[tokenCount].str = ">=";
                    i += 2;
                } else if (fileContent[i + 1] == '>') {
                    tokens[tokenCount].type = tt_rshift;
                    tokens[tokenCount].str = ">>";
                    i += 2;
                } else {
                    tokens[tokenCount].type = tt_gt;
                    tokens[tokenCount].str = ">";
                    i++;
                }
                break;
            }
            case '(': {
                tokens[tokenCount].type = tt_lparen;
                tokens[tokenCount].str = "(";
                i++;
                break;
            }
            case ')': {
                tokens[tokenCount].type = tt_rparen;
                tokens[tokenCount].str = ")";
                i++;
                break;
            }
            case '[': {
                tokens[tokenCount].type = tt_lbracket;
                tokens[tokenCount].str = "[";
                i++;
                break;
            }
            case ']': {
                tokens[tokenCount].type = tt_rbracket;
                tokens[tokenCount].str = "]";
                i++;
                break;
            }
            case '{': {
                // remove the last token if it is a newline
                if (tokens[tokenCount - 1].type == tt_endl) {
                    tokenCount--;
                }
                tokens[tokenCount].type = tt_lbrace;
                tokens[tokenCount].str = "{";
                i++;
                break;
            }
            case '}': {
                // remove the last token if it is a newline
                if (tokens[tokenCount - 1].type == tt_endl) {
                    tokenCount--;
                }
                tokens[tokenCount].type = tt_rbrace;
                tokens[tokenCount].str = "}";
                i++;
                break;
            }
            case ',': {
                tokens[tokenCount].type = tt_comma;
                tokens[tokenCount].str = ",";
                i++;
                if (fileContent[i] == '\n') {
                    iOfLastOveride = iOfLastNewLine;
                    iOfLastNewLine = i;
                    lineNumber++;
                    i++;
                }
                break;
            }
            case ';': {
                tokens[tokenCount].type = tt_semi;
                tokens[tokenCount].str = ";";
                i++;
                break;
            }
            case ':': {
                tokens[tokenCount].type = tt_colon;
                tokens[tokenCount].str = ":";
                i++;
                break;
            }
            case '\n': {
                iOfLastNewLine = i;
                tokens[tokenCount].type = tt_endl;
                tokens[tokenCount].str = "\n";
                lineNumber++;
                i++;
                break;
            }
            case '.': {
                if (fileContent[i + 1] == '.' && fileContent[i + 1] != '\0' && fileContent[i + 2] == '.') {
                    tokens[tokenCount].type = tt_elips;
                    tokens[tokenCount].str = "...";
                    i += 3;
                } else {
                    tokens[tokenCount].type = tt_dot;
                    tokens[tokenCount].str = ".";
                    i++;
                }
                break;
            }
            default: {
                logError("Unknown character '%c' at line %u", fileContent[i], lineNumber);
                tokens[tokenCount].type = tt_error;
                tokens[tokenCount].str = "error";
                i++;
                break;
            }
        }
        tokens[tokenCount].charStart = charStart;
        tokens[tokenCount].charEnd = i - iOfLastNewLine - 2;
        if (iOfLastOveride != UINT64_MAX) {
            tokens[tokenCount].charEnd = i - iOfLastOveride - 2;
        }
        tokens[tokenCount].file = fileNumber;
        tokens[tokenCount].line = lineNumber;
        tokenCount++;
        break;
    }


    *iRef = i;
    *lineNumberRef = lineNumber;
    *charPosRef = iOfLastNewLine;
    *tokenCountRef = tokenCount;
    *tokenCapacityRef = tokenCapacity;
}

Token* loadTokensFromFile(char* fileContent, Arena* arena, u64 fileNumber) {
    u64 tokenCapacity = 1024;
    Token* tokens = (Token*)arenaAlloc(arena, sizeof(Token) * 1024);
    u64 tokenCount = 0;
    u64 i = 0;
    u64 lineNumber = 1;
    u64 charPos = 0;
    while (fileContent[i] != '\0') {
        addToken(fileContent, fileNumber, &i, tokens, &tokenCount, &tokenCapacity, &lineNumber, &charPos, arena);
    }
    if (tokenCount >= tokenCapacity) {
        tokens = arenaRealloc(arena, tokens, sizeof(Token) * tokenCapacity, sizeof(Token) * tokenCapacity * 2);
        tokenCapacity *= 2;
    }
    tokens[tokenCount++] = (Token) { tt_eof, fileNumber, lineNumber - 1, UINT16_MAX, UINT16_MAX, "EOF" };

    // set charStart and charEnd for non Normal tokens
    Token* token = tokens;
    while (token->type != tt_eof) {
        if (token->type == tt_endl) {
            token->charStart = UINT16_MAX;
            token->charEnd = UINT16_MAX;
            token->line--;
        }
        token++;
    }
    return tokens;
}
