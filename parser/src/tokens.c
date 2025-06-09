#include "parser/tokens.h"
#include "parser.h"
#include <assert.h>
#include <stdio.h>
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

Token* loadTokensFromDirectory(char** sourceFiles, u64 fileCount, Arena* arena) {
    u64 tokenCapacity = 1024 * fileCount;
    Token* tokens = (Token*)arenaAlloc(arena, sizeof(Token) * 1024 * fileCount);
    u64 tokenCount = 0;
    for (u64 fileNumber = 0; fileNumber < fileCount; fileNumber++) {
        char* fileContent = sourceFiles[fileNumber];
        u64 i = 0;
        u64 lineNumber = 1;
        while (fileContent[i] != '\0') {
            if (fileContent[i] == '\r' || fileContent[i] == ' ' || fileContent[i] == '\t') {
                i++;
                continue;
            }
            if (fileContent[i] == '/' && fileContent[i + 1] == '/') {
                // single line comment
                while (fileContent[i] != '\n') {
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
                tokenCapacity *= 2;
                Token* tempTokens = (Token*)arenaAlloc(arena, sizeof(Token) * tokenCapacity);
                memcpy(tempTokens, tokens, sizeof(Token) * (tokenCount - 1));
                tokens = tempTokens;
            }
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
                    tokens[tokenCount++] = (Token) { type, fileNumber, lineNumber, start, i - 1, identifier };
                    break;
                }
                case '"': {
                    // String literal
                    u64 start = i;
                    i++;  // Skip the opening quote
                    while (fileContent[i] != '"' && fileContent[i] != '\0') {
                        if (fileContent[i] == '\\') {
                            i += 2;  // Skip the escape sequence
                        } else {
                            i++;  // Skip the character
                        }
                    }
                    if (fileContent[i] == '\0') {
                        logError("Unterminated string literal starting at line %u", lineNumber);
                        break;
                    }
                    i++;  // Skip the closing quote
                    u64 length = i - start - 2;
                    char* stringLiteral = (char*)arenaAlloc(arena, length + 1);
                    memcpy(stringLiteral, &fileContent[start] + 1, length);
                    stringLiteral[length] = '\0';
                    tokens[tokenCount++] = (Token) { tt_string, fileNumber, lineNumber, start, i - 1, stringLiteral };
                    break;
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
                    tokens[tokenCount++] = (Token) { type, fileNumber, lineNumber, start, i - 1, numberStr };
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
                    tokens[tokenCount++] = (Token) { tt_char, fileNumber, lineNumber, start, i - 1, charStr };
                    break;
                }
                case '+': {
                    if (fileContent[i + 1] == '+') {
                        tokens[tokenCount++] = (Token) { tt_inc, fileNumber, lineNumber, i, i + 1, "++" };
                        i += 2;
                    } else if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_add_assign, fileNumber, lineNumber, i, i + 1, "+=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_add, fileNumber, lineNumber, i, i, "+" };
                        i++;
                    }
                    break;
                }
                case '-': {
                    if (fileContent[i + 1] == '-') {
                        tokens[tokenCount++] = (Token) { tt_dec, fileNumber, lineNumber, i, i + 1, "--" };
                        i += 2;
                    } else if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_sub_assign, fileNumber, lineNumber, i, i + 1, "-=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_sub, fileNumber, lineNumber, i, i, "-" };
                        i++;
                    }
                    break;
                }
                case '*': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_mul_assign, fileNumber, lineNumber, i, i + 1, "*=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_mul, fileNumber, lineNumber, i, i, "*" };
                        i++;
                    }
                    break;
                }
                case '/': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_div_assign, fileNumber, lineNumber, i, i + 1, "/=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_div, fileNumber, lineNumber, i, i, "/" };
                        i++;
                    }
                    break;
                }
                case '%': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_mod_assign, fileNumber, lineNumber, i, i + 1, "%=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_mod, fileNumber, lineNumber, i, i, "%" };
                        i++;
                    }
                    break;
                }
                case '&': {
                    if (fileContent[i + 1] == '&') {
                        tokens[tokenCount++] = (Token) { tt_and, fileNumber, lineNumber, i, i + 1, "&&" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_bit_and, fileNumber, lineNumber, i, i, "&" };
                        i++;
                    }
                    break;
                }
                case '|': {
                    if (fileContent[i + 1] == '|') {
                        tokens[tokenCount++] = (Token) { tt_or, fileNumber, lineNumber, i, i + 1, "||" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_bit_or, fileNumber, lineNumber, i, i, "|" };
                        i++;
                    }
                    break;
                }
                case '^': {
                    tokens[tokenCount++] = (Token) { tt_xor, fileNumber, lineNumber, i, i, "^" };
                    i++;
                    break;
                }
                case '!': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_neq, fileNumber, lineNumber, i, i + 1, "!=" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_not, fileNumber, lineNumber, i, i, "!" };
                        i++;
                    }
                    break;
                }
                case '=': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_eq, fileNumber, lineNumber, i, i + 1, "==" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_assign, fileNumber, lineNumber, i, i, "=" };
                        i++;
                    }
                    break;
                }
                case '<': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_leq, fileNumber, lineNumber, i, i + 1, "<=" };
                        i += 2;
                    } else if (fileContent[i + 1] == '<') {
                        tokens[tokenCount++] = (Token) { tt_lshift, fileNumber, lineNumber, i, i + 1, "<<" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_lt, fileNumber, lineNumber, i, i, "<" };
                        i++;
                    }
                    break;
                }
                case '>': {
                    if (fileContent[i + 1] == '=') {
                        tokens[tokenCount++] = (Token) { tt_geq, fileNumber, lineNumber, i, i + 1, ">=" };
                        i += 2;
                    } else if (fileContent[i + 1] == '>') {
                        tokens[tokenCount++] = (Token) { tt_rshift, fileNumber, lineNumber, i, i + 1, ">>" };
                        i += 2;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_gt, fileNumber, lineNumber, i, i, ">" };
                        i++;
                    }
                    break;
                }
                case '(': {
                    tokens[tokenCount++] = (Token) { tt_lparen, fileNumber, lineNumber, i, i, "(" };
                    i++;
                    break;
                }
                case ')': {
                    tokens[tokenCount++] = (Token) { tt_rparen, fileNumber, lineNumber, i, i, ")" };
                    i++;
                    break;
                }
                case '[': {
                    tokens[tokenCount++] = (Token) { tt_lbracket, fileNumber, lineNumber, i, i, "[" };
                    i++;
                    break;
                }
                case ']': {
                    tokens[tokenCount++] = (Token) { tt_rbracket, fileNumber, lineNumber, i, i, "]" };
                    i++;
                    break;
                }
                case '{': {
                    tokens[tokenCount++] = (Token) { tt_lbrace, fileNumber, lineNumber, i, i, "{" };
                    i++;
                    break;
                }
                case '}': {
                    tokens[tokenCount++] = (Token) { tt_rbrace, fileNumber, lineNumber, i, i, "}" };
                    i++;
                    break;
                }
                case ',': {
                    tokens[tokenCount++] = (Token) { tt_comma, fileNumber, lineNumber, i, i, "," };
                    i++;
                    break;
                }
                case ';': {
                    tokens[tokenCount++] = (Token) { tt_semi, fileNumber, lineNumber, i, i, ";" };
                    i++;
                    break;
                }
                case ':': {
                    tokens[tokenCount++] = (Token) { tt_colon, fileNumber, lineNumber, i, i, ":" };
                    i++;
                    break;
                }
                case '\n': {
                    tokens[tokenCount++] = (Token) { tt_endl, fileNumber, lineNumber, i, i, "\n" };
                    lineNumber++;
                    i++;
                    break;
                }
                case '.': {
                    if (fileContent[i + 1] == '.' && fileContent[i + 1] != '\0' && fileContent[i + 2] == '.') {
                        tokens[tokenCount++] = (Token) { tt_elips, fileNumber, lineNumber, i, i + 1, "..." };
                        i += 3;
                    } else {
                        tokens[tokenCount++] = (Token) { tt_dot, fileNumber, lineNumber, i, i, "." };
                        i++;
                    }
                    break;
                }
                default: {
                    logError("Unknown character '%c' at line %u", fileContent[i], lineNumber);
                    tokens[tokenCount++] = (Token) { tt_error, fileNumber, lineNumber, i, i, "error" };
                    i++;
                    break;
                }
            }
        }
        // Add EOF token for each file
        if (tokenCount >= tokenCapacity) {
            tokenCapacity *= 2;
            Token* tempTokens = (Token*)arenaAlloc(arena, sizeof(Token) * tokenCapacity);
            memcpy(tempTokens, tokens, sizeof(Token) * (tokenCount - 1));
            tokens = tempTokens;
        }
        tokens[tokenCount++] = (Token) { tt_eof, fileNumber, lineNumber, i, i, "EOF" };
    }
    // add end of project token
    if (tokenCount >= tokenCapacity) {
        tokenCapacity *= 2;
        Token* tempTokens = (Token*)arenaAlloc(arena, sizeof(Token) * tokenCapacity);
        memcpy(tempTokens, tokens, sizeof(Token) * (tokenCount - 1));
        tokens = tempTokens;
    }
    tokens[tokenCount++] = (Token) { tt_eop, 0, 0, 0, 0, "EOP" };
    return tokens;
}
