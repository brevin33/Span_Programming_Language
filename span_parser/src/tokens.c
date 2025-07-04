#include "span_parser.h"
#include <assert.h>

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

Token* createTokens(Arena arena, char* fileContents, u64 fileIndex) {
    u64 tokenCount = 0;
    u64 tokensCapacity = 8;
    Token* tokens = allocArena(arena, sizeof(Token) * tokensCapacity);
    u32 index = 0;
    char stringStack[512];
    memset(stringStack, 0, sizeof(stringStack));
    u64 stringStackIndex = 0;
    while (fileContents[index] != '\0') {
        if (tokenCount >= tokensCapacity) {
            tokens = reallocArena(arena, sizeof(Token) * (tokensCapacity * 2), tokens, sizeof(Token) * tokensCapacity);
            tokensCapacity *= 2;
        }
        massert(index != UINT32_MAX, "Token index overflow");
        Token token = createToken(fileContents, &index, fileIndex, stringStack, &stringStackIndex);
        tokens[tokenCount++] = token;
    }
    return tokens;
}

static bool isIdChar(char c) {
    switch (c) {
        case '_':
        CASE_LETTER:
            return true;
        default:
            return false;
    }
}

static bool isNumberChar(char c) {
    switch (c) {
    CASE_NUMBER:
        return true;
        default:
            return false;
    }
}

static void handelKeyword(Token* token, char* id) {
    if (strcmp(id, "return") == 0) {
        token->type = tt_return;
        return;
    }
    if (strcmp(id, "if") == 0) {
        token->type = tt_if;
        return;
    }
    if (strcmp(id, "for") == 0) {
        token->type = tt_for;
        return;
    }
    if (strcmp(id, "struct") == 0) {
        token->type = tt_struct;
        return;
    }
    if (strcmp(id, "enum") == 0) {
        token->type = tt_enum;
        return;
    }
    if (strcmp(id, "union") == 0) {
        token->type = tt_union;
        return;
    }
    if (strcmp(id, "interface") == 0) {
        token->type = tt_interface;
        return;
    }
}


static char* getNumberString(char* fileContent, u32* indexRef, char* buffer) {
    u64 numberBufferIndex = 0;
    u32 index = *indexRef;

    while (isNumberChar(fileContent[index])) {
        buffer[numberBufferIndex++] = fileContent[index++];
    }
    if (fileContent[index] == '.') {
        index++;
        while (isNumberChar(fileContent[index])) {
            buffer[numberBufferIndex++] = fileContent[index++];
        }
    }
    buffer[numberBufferIndex] = '\0';
    *indexRef = index;
    return buffer;
}

static char* getStringString(char* fileContent, u32* indexRef, char* buffer, u16 fileIndex) {
    u64 stringBufferIndex = 0;
    u32 index = *indexRef;
    index++;  // skip "
    while (fileContent[index] != '\n') {
        if (fileContent[index] == '"') {
            index++;
            break;
        }
        buffer[stringBufferIndex++] = fileContent[index++];
    }
    return buffer;
}

static void handelIdOrKeyword(char* fileContent, u32* indexRef, Token* token) {
    u32 index = *indexRef;
    char buffer[4096];
    u32 idBufferIndex = 0;
    while (isIdChar(fileContent[index])) {
        buffer[idBufferIndex++] = fileContent[index++];
    }
    token->type = tt_id;
    handelKeyword(token, buffer);
    *indexRef = index;
}

static void handelNumber(char* fileContent, u32* indexRef, Token* token) {
    u32 index = *indexRef;
    token->type = tt_number;
    char numberBuffer[4096];
    getNumberString(fileContent, &index, numberBuffer);
    *indexRef = index;
}

static bool isMultiLineString(char* fileContent, u32* indexRef, char* stringStack, u64* stringStackIndexRef) {
    u64 stringStackIndex = *stringStackIndexRef;
    u32 index = *indexRef;
    bool isMultiLine = false;
    if (fileContent[index] == '}') {
        if (stringStack[stringStackIndex] == 1) {
            isMultiLine = false;
        } else if (stringStack[stringStackIndex] == 2) {
            isMultiLine = true;
        } else {
            massert(false, "string stack corrupted");
        }
        stringStack[stringStackIndex] = 0;
        stringStackIndex--;
        index++;
    } else if (fileContent[index] == '"') {
        index++;
        if (fileContent[index] == '"' && fileContent[index + 1] == '"') {
            index += 2;
            isMultiLine = true;
        }
    } else {
        massert(false, "Expected \" at start of string");
    }
    *indexRef = index;
    *stringStackIndexRef = stringStackIndex;
    return isMultiLine;
}

static void handelString(char* fileContent, u32* indexRef, Token* token, char* stringStack, u64* stringStackIndexRef) {
    u32 index = *indexRef;
    u64 stringStackIndex = *stringStackIndexRef;
    bool isMultiLine = isMultiLineString(fileContent, &index, stringStack, &stringStackIndex);
    bool stringTerminated = false;
    bool stringContinue = false;
    while (true) {
        char c = fileContent[index];
        if (fileContent[index] == '"' && (fileContent[index + 1] != '"' || fileContent[index + 2] != '"') && isMultiLine) {
            SpanProject* project = context.activeProject;
            SpanFile* file = &project->files[token->file];
            char* fileContent = file->fileContents;
            char* internalPointer = fileContent + index;
            u64 line = SpanFileFindLineFromInternalPointer(file, internalPointer);
            char* fileName = file->fileName;
            logError("At line %llu in file %s. Can't directly have \" in a multi-line string. use \"\" instead", line, fileName);
        }
        if (fileContent[index] == '\\' && (fileContent[index + 1] == '"' || fileContent[index + 1] == '{')) {
            index += 2;
            continue;
        } else if (fileContent[index] == '"' && !isMultiLine) {
            stringTerminated = true;
            index++;
            break;
        } else if (fileContent[index] == '"' && fileContent[index + 1] == '"' && fileContent[index + 2] == '"' && isMultiLine) {
            stringTerminated = true;
            index += 3;
            break;
        } else if (fileContent[index] == '\n' && !isMultiLine) {
            break;
        } else if (fileContent[index] == '\0') {
            break;
        } else if (fileContent[index] == '{') {
            stringStackIndex++;
            if (!isMultiLine) {
                stringStack[stringStackIndex] = 1;
            } else {
                stringStack[stringStackIndex] = 2;
            }
            stringTerminated = true;
            stringContinue = true;
            index++;
            break;
        }
        index++;
    }
    if (!stringTerminated) {
        token->type = tt_invalid;
        SpanProject* project = context.activeProject;
        SpanFile* file = &project->files[token->file];
        char* fileContent = file->fileContents;
        char* internalPointer = fileContent + token->tokenStart;
        u64 line = SpanFileFindLineFromInternalPointer(file, internalPointer);
        char* fileName = file->fileName;
        logError("unterminated string at line %llu in file %s", line, fileName);
    } else if (stringContinue) {
        token->type = tt_string_continue;
    } else {
        token->type = tt_string_end;
    }
    *indexRef = index;
    *stringStackIndexRef = stringStackIndex;
}

char* ourTokenTypeToString(OurTokenType type) {
    switch (type) {
        case tt_invalid:
            return "invalid";
        case tt_return:
            return "return";
        case tt_id:
            return "id";
        case tt_dot:
            return "dot";
        case tt_number:
            return "number";
        case tt_string_end:
            return "string_end";
        case tt_string_continue:
            return "string_continue";
        case tt_rbrace:
            return "rbrace";
        case tt_lbrace:
            return "lbrace";
        case tt_endl:
            return "endl";
        case tt_comma:
            return "comma";
        case tt_colon:
            return "colon";
        case tt_add:
            return "add";
        case tt_sub:
            return "sub";
        case tt_mul:
            return "mul";
        case tt_div:
            return "div";
        case tt_mod:
            return "mod";
        case tt_eq:
            return "eq";
        case tt_neq:
            return "neq";
        case tt_lt:
            return "lt";
        case tt_gt:
            return "gt";
        case tt_le:
            return "le";
        case tt_ge:
            return "ge";
        case tt_lparen:
            return "lparen";
        case tt_rparen:
            return "rparen";
        case tt_lbracket:
            return "lbracket";
        case tt_rbracket:
            return "rbracket";
        case tt_assign:
            return "assign";
        case tt_assign_add:
            return "assign_add";
        case tt_assign_sub:
            return "assign_sub";
        case tt_assign_mul:
            return "assign_mul";
        case tt_assign_div:
            return "assign_div";
        case tt_assign_mod:
            return "assign_mod";
        case tt_assign_infer:
            return "assign_infer";
        case tt_eof:
            return "eof";
        case tt_colon_colon:
            return "colon_colon";
        case tt_negate:
            return "negate";
        case tt_uptr:
            return "uptr";
        case tt_sptr:
            return "sptr";
        case tt_char:
            return "char";
        case tt_if:
            return "if";
        case tt_for:
            return "for";
        case tt_struct:
            return "struct";
        case tt_enum:
            return "enum";
        case tt_union:
            return "union";
        case tt_interface:
            return "interface";
    }
    return "no string add for this token type";
}

Token createToken(char* fileContent, u32* indexRef, u16 fileIndex, char* stringStack, u64* stringStackIndexRef) {
    Token token;
    u32 index = *indexRef;
    u64 stringStackIndex = *stringStackIndexRef;
    token.file = fileIndex;
    while (true) {
        token.tokenStart = index;  // this is inside the loop because we need to update after end lines and stuff
        switch (fileContent[index]) {
            case '_':
            CASE_LETTER: {
                handelIdOrKeyword(fileContent, &index, &token);
                break;
            }
            case '.': {
                if (isNumberChar(fileContent[index + 1])) {
                    handelNumber(fileContent, &index, &token);
                    break;
                }
                token.type = tt_dot;
                index++;
                break;
            }
            CASE_NUMBER: {
                handelNumber(fileContent, &index, &token);
                break;
            }
            case '"': {
                handelString(fileContent, &index, &token, stringStack, &stringStackIndex);
                break;
            }
            case '}': {
                if (stringStack[stringStackIndex] == 1 || stringStack[stringStackIndex] == 2) {
                    handelString(fileContent, &index, &token, stringStack, &stringStackIndex);
                } else {
                    if (stringStackIndex > 0) {
                        stringStackIndex--;
                    }
                    token.type = tt_rbrace;
                    index++;
                }
                break;
            }
            case '{': {
                stringStackIndex++;
                stringStack[stringStackIndex] = 0;
                token.type = tt_lbrace;
                index++;
                break;
            }
            case '\n': {
                token.type = tt_endl;
                index++;
                break;
            }
            case ' ':
            case '\t':
            case '\r': {
                index++;
                continue;
            }
            case ',': {
                token.type = tt_comma;
                index++;
                if (fileContent[index] == '\n') {
                    index++;
                }
                break;
            }
            case ':': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_infer;
                    index++;
                    break;
                } else if (fileContent[index] == ':') {
                    token.type = tt_colon_colon;
                    index++;
                    break;
                }
                token.type = tt_colon;
                break;
            }
            case ';': {
                token.type = tt_endl;
                index++;
                break;
            }
            case '+': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_add;
                    index++;
                    break;
                }
                token.type = tt_add;
                break;
            }
            case '-': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_sub;
                    index++;
                    break;
                }
                token.type = tt_sub;
                break;
            }
            case '*': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_mul;
                    index++;
                    break;
                }
                token.type = tt_mul;
                break;
            }
            case '/': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_div;
                    index++;
                    break;
                }
                token.type = tt_div;
                break;
            }
            case '%': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_assign_mod;
                    index++;
                    break;
                }
                token.type = tt_mod;
                break;
            }
            case '=': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_eq;
                    index++;
                    break;
                }
                token.type = tt_assign;
                break;
            }
            case '!': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_neq;
                    index++;
                    break;
                }
                token.type = tt_negate;
                break;
            }
            case '<': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_le;
                    index++;
                    break;
                }
                token.type = tt_lt;
                break;
            }
            case '>': {
                index++;
                if (fileContent[index] == '=') {
                    token.type = tt_ge;
                    index++;
                    break;
                }
                token.type = tt_gt;
                break;
            }
            case '(': {
                token.type = tt_lparen;
                index++;
                break;
            }
            case ')': {
                token.type = tt_rparen;
                index++;
                break;
            }
            case '[': {
                token.type = tt_lbracket;
                index++;
                break;
            }
            case ']': {
                token.type = tt_rbracket;
                index++;
                break;
            }
            case '^': {
                token.type = tt_uptr;
                index++;
                break;
            }
            case '\'': {
                index++;
                if (fileContent[index] == '\\') {
                    if (fileContent[index + 1] != '\0' && fileContent[index + 2] == '\'') {
                        index += 3;
                        token.type = tt_char;
                        break;
                    } else {
                        token.type = tt_sptr;
                        break;
                    }
                }
                if (fileContent[index] == '\'') {
                    token.type = tt_sptr;
                    break;
                }
                if (fileContent[index] == '\0') {
                    token.type = tt_sptr;
                    break;
                }
                if (fileContent[index + 1] == '\'') {
                    index += 2;
                    token.type = tt_char;
                    break;
                }
                token.type = tt_sptr;
                break;
            }
            default: {
                token.type = tt_invalid;
                index++;
                break;
            }
        }
        break;
    }
    u32 tokenLength = index - token.tokenStart;
    tokenLength = tokenLength > UINT8_MAX ? UINT8_MAX : tokenLength;
    token.tokenLength = tokenLength;
    *indexRef = index;
    *stringStackIndexRef = stringStackIndex;
    return token;
}

void handelStringEscape(char* string, Token token) {
    // will do this in place
    char* newString = string;
    while (*string != '\0') {
        if (*string == '\\') {
            string++;
            switch (*string) {
                case 'n':
                    *newString = '\n';
                    break;
                case 't':
                    *newString = '\t';
                    break;
                case 'r':
                    *newString = '\r';
                    break;
                case '\\':
                    *newString = '\\';
                    break;
                case '"':
                    *newString = '"';
                    break;
                case '\'':
                    *newString = '\'';
                    break;
                case '0':
                    *newString = '\0';
                    break;
                case 'a':
                    *newString = '\a';
                    break;
                case 'b':
                    *newString = '\b';
                    break;
                case 'f':
                    *newString = '\f';
                    break;
                case 'v':
                    *newString = '\v';
                    break;
                case '{':
                    *newString = '{';
                    break;
                case '}':
                    *newString = '}';
                    break;
                default:
                    logErrorTokens(&token, 1, "unknown escape: \\%c", *string);
                    *newString = *(string - 1);
                    newString++;
                    *newString = *string;
                    break;
            }
        } else {
            *newString = *string;
        }
        string++;
        newString++;
    }
    *newString = '\0';
}

char tokenGetTypeChar(Token token) {
    massert(token.type == tt_char, "not a char");
    SpanProject* project = context.activeProject;
    SpanFile* file = &project->files[token.file];
    char* fileContent = file->fileContents;
    u32 index = token.tokenStart;
    assert(fileContent[index] == '\'');
    index++;
    if (fileContent[index] == '\\') {
        index++;
        switch (fileContent[index]) {
            case 'n':
                return '\n';
            case 't':
                return '\t';
            case 'r':
                return '\r';
            case '\\':
                return '\\';
            case '"':
                return '"';
            case '\'':
                return '\'';
            case '0':
                return '\0';
            case 'a':
                return '\a';
            case 'b':
                return '\b';
            case 'f':
                return '\f';
            case 'v':
                return '\v';
            case '{':
                return '{';
            case '}':
                return '}';
            default: {
                logErrorTokens(&token, 1, "unknown escape: \\%c", fileContent[index]);
                return '\0';
            }
        }
    }
    return fileContent[index];
}

char* tokenGetString(Token token, char* buffer) {
    SpanProject* project = context.activeProject;
    SpanFile* file = &project->files[token.file];
    char* fileContent = file->fileContents;
    u32 index = token.tokenStart;
    switch (token.type) {
        case tt_id: {
            u32 idBufferIndex = 0;
            while (isIdChar(fileContent[index])) {
                buffer[idBufferIndex++] = fileContent[index++];
            }
            buffer[idBufferIndex] = '\0';
            return buffer;
        }
        case tt_number: {
            return getNumberString(fileContent, &index, buffer);
        }
        case tt_string_end:
        case tt_string_continue: {
            // skip opening
            if (fileContent[index] == '}') {
                index++;
            } else if (fileContent[index] == '"' && fileContent[index + 1] == '"' && fileContent[index + 2] == '"') {
                index += 3;
            } else if (fileContent[index] == '"') {
                index++;
            } else {
                massert(false, "unexpected string start");
            }

            u32 stringBufferIndex = 0;
            while (true) {
                if (fileContent[index] == '\\' && (fileContent[index + 1] == '"' || fileContent[index + 1] == '{')) {
                    index++;
                    buffer[stringBufferIndex++] = fileContent[index++];
                    continue;
                } else if (fileContent[index] == '"') {
                    break;
                } else if (fileContent[index] == '{') {
                    break;
                }
                buffer[stringBufferIndex++] = fileContent[index++];
            }
            buffer[stringBufferIndex] = '\0';
            handelStringEscape(buffer, token);
            return buffer;
        }
        default: {
            massert(false, "trying to get string for token type without string");
            return "";
        }
    }
}

void tokenGetLineColumn(Token token, u64* outLine, u64* outColumnStart, u64* outColumnEnd) {
    SpanProject* project = context.activeProject;
    SpanFile* file = &project->files[token.file];
    char* fileContent = file->fileContents;
    char* internalPointer = fileContent + token.tokenStart;
    u64 line = SpanFileFindLineFromInternalPointer(file, internalPointer);
    u64 columnStart = (token.tokenStart + fileContent) - file->fileLineStarts[line];
    u64 columnEnd = columnStart + token.tokenLength;
    *outLine = line;
    *outColumnStart = columnStart;
    *outColumnEnd = columnEnd;
}
