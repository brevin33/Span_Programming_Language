#include "parser/tokens.h"
#include "parser.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

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
#define CASE_DIGIT                                                                                                                                                                                                                   \
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



std::string to_string(const TokenType& type) {
    switch (type) {
        case tt_id:
            return "tt_id";
        case tt_int:
            return "tt_int";
        case tt_float:
            return "tt_float";
        case tt_string:
            return "tt_string";
        case tt_char:
            return "tt_char";
        case tt_comment:
            return "tt_comment";
        case tt_lparen:
            return "tt_lparen";
        case tt_rparen:
            return "tt_rparen";
        case tt_lbrace:
            return "tt_lbrace";
        case tt_rbrace:
            return "tt_rbrace";
        case tt_lbracket:
            return "tt_lbracket";
        case tt_rbracket:
            return "tt_rbracket";
        case tt_semicolon:
            return "tt_semicolon";
        case tt_colon:
            return "tt_colon";
        case tt_comma:
            return "tt_comma";
        case tt_plus:
            return "tt_plus";
        case tt_minus:
            return "tt_minus";
        case tt_multiply:
            return "tt_multiply";
        case tt_divide:
            return "tt_divide";
        case tt_modulus:
            return "tt_modulus";
        case tt_equal:
            return "tt_equal";
        case tt_assign:
            return "tt_assign";
        case tt_not_equal:
            return "tt_not_equal";
        case tt_less_than:
            return "tt_less_than";
        case tt_greater_than:
            return "tt_greater_than";
        case tt_less_than_equal:
            return "tt_less_than_equal";
        case tt_greater_than_equal:
            return "tt_greater_than_equal";
        case tt_and:
            return "tt_and";
        case tt_or:
            return "tt_or";
        case tt_xor:
            return "tt_xor";
        case tt_bitwise_and:
            return "tt_bitwise_and";
        case tt_bitwise_or:
            return "tt_bitwise_or";
        case tt_bitwise_xor:
            return "tt_bitwise_xor";
        case tt_bitwise_not:
            return "tt_bitwise_not";
        case tt_shift_left:
            return "tt_shift_left";
        case tt_shift_right:
            return "tt_shift_right";
        case tt_error:
            return "tt_error";
        case tt_return:
            return "tt_return";
        case tt_struct:
            return "tt_struct";
        case tt_if:
            return "tt_if";
        case tt_else:
            return "tt_else";
        case tt_while:
            return "tt_while";
        case tt_for:
            return "tt_for";
        case tt_switch:
            return "tt_switch";
        case tt_case:
            return "tt_case";
        case tt_sptr:
            return "tt_sptr";
        case tt_not:
            return "tt_not";
        case tt_endline:
            return "tt_endline";
        case tt_endfile:
            return "tt_endfile";
        default:
            return "unknown";
    }
}
std::string to_string(const Token& token) {
    stringstream ss;
    if (token.type == tt_id || token.type == tt_int || token.type == tt_float) {
        ss << token.str;
    } else if (token.type == tt_string) {
        ss << "\"" << token.str << "\"";
    } else if (token.type == tt_char) {
        ss << "'" << token.str << "'";
    } else {
        ss << to_string(token.type);
    }
    string result = ss.str();
    return result;
}

std::string to_string(const Tokens& tokens) {
    stringstream ss;
    for (const auto& line : tokens.tokensByLine) {
        if (line.front().type == tt_endline) {
            continue;  // Skip empty lines
        }
        for (const auto& token : line) {
            ss << to_string(token) << " | ";
        }
        ss << endl;
    }
    string result = ss.str();
    return result;
}

void changeIfKeyword(Token& s) {
    if (strcmp(s.str, "return") == 0) {
        s.type = tt_return;
    } else if (strcmp(s.str, "if") == 0) {
        s.type = tt_if;
    } else if (strcmp(s.str, "else") == 0) {
        s.type = tt_else;
    } else if (strcmp(s.str, "while") == 0) {
        s.type = tt_while;
    } else if (strcmp(s.str, "for") == 0) {
        s.type = tt_for;
    } else if (strcmp(s.str, "switch") == 0) {
        s.type = tt_switch;
    } else if (strcmp(s.str, "case") == 0) {
        s.type = tt_case;
    }
}


Token createToken(string line, u64& i, Tokens& s) {
    Token token;
    token.charStart = i;
    switch (line[i]) {
        case '_':
        CASE_LETTER: {
            token.type = tt_id;
            while (i < line.size() && (line[i] == '_' || isalnum(line[i]))) {
                ++i;
            }
            u64 size = i - token.charStart;
            token.str = (char*)s.arena.alloc(size + 1);
            memcpy(token.str, line.c_str() + token.charStart, size);
            token.str[size] = '\0';
            token.charEnd = i;
            changeIfKeyword(token);
            break;
        }
        CASE_DIGIT: {
            bool isFloat = false;
            while (i < line.size() && isdigit(line[i])) {
                ++i;
            }
            if (i < line.size() && line[i] == '.') {
                isFloat = true;
                ++i;
                while (i < line.size() && isdigit(line[i])) {
                    ++i;
                }
            }
            if (isFloat) {
                token.type = tt_float;
            } else {
                token.type = tt_int;
            }
            u64 size = i - token.charStart;
            token.str = (char*)s.arena.alloc(size + 1);
            memcpy(token.str, line.c_str() + token.charStart, size);
            token.str[size] = '\0';
            token.charEnd = i;
            break;
        }
        case '"': {
            token.type = tt_string;
            ++i;
            while (i < line.size() && line[i] != '"') {
                ++i;
            }
            if (i < line.size()) {
                ++i;
            } else {
                token.type = tt_error;
                token.charEnd = i;
                return token;
            }
            u64 size = i - token.charStart - 2;
            token.str = (char*)s.arena.alloc(size + 1);
            memcpy(token.str, line.c_str() + token.charStart + 1, size);
            token.str[size] = '\0';
            token.charEnd = i;
            break;
        }
        case '(': {
            token.type = tt_lparen;
            ++i;
            token.charEnd = i;
            break;
        }
        case ')': {
            token.type = tt_rparen;
            ++i;
            token.charEnd = i;
            break;
        }
        case '{': {
            token.type = tt_lbrace;
            ++i;
            token.charEnd = i;
            break;
        }
        case '}': {
            token.type = tt_rbrace;
            ++i;
            token.charEnd = i;
            break;
        }
        case '[': {
            token.type = tt_lbracket;
            ++i;
            token.charEnd = i;
            break;
        }
        case ']': {
            token.type = tt_rbracket;
            ++i;
            token.charEnd = i;
            break;
        }
        case ';': {
            token.type = tt_semicolon;
            ++i;
            token.charEnd = i;
            break;
        }
        case ':': {
            token.type = tt_colon;
            ++i;
            token.charEnd = i;
            break;
        }
        case ',': {
            token.type = tt_comma;
            ++i;
            token.charEnd = i;
            break;
        }
        case '+': {
            token.type = tt_plus;
            ++i;
            token.charEnd = i;
            break;
        }
        case '-': {
            token.type = tt_minus;
            ++i;
            token.charEnd = i;
            break;
        }
        case '*': {
            token.type = tt_multiply;
            ++i;
            token.charEnd = i;
            break;
        }
        case '/': {
            token.type = tt_divide;
            ++i;
            token.charEnd = i;
            break;
        }
        case '%': {
            token.type = tt_modulus;
            ++i;
            token.charEnd = i;
            break;
        }
        case '=': {
            if (i + 1 < line.size() && line[i + 1] == '=') {
                token.type = tt_equal;
                i += 2;
            } else {
                token.type = tt_assign;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '!': {
            if (i + 1 < line.size() && line[i + 1] == '=') {
                token.type = tt_not_equal;
                i += 2;
            } else {
                token.type = tt_not;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '<': {
            if (i + 1 < line.size() && line[i + 1] == '=') {
                token.type = tt_less_than_equal;
                i += 2;
            } else if (i + 1 < line.size() && line[i + 1] == '<') {
                token.type = tt_shift_left;
                i += 2;
            } else {
                token.type = tt_less_than;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '>': {
            if (i + 1 < line.size() && line[i + 1] == '=') {
                token.type = tt_greater_than_equal;
                i += 2;
            } else if (i + 1 < line.size() && line[i + 1] == '>') {
                token.type = tt_shift_right;
                i += 2;
            } else {
                token.type = tt_greater_than;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '&': {
            if (i + 1 < line.size() && line[i + 1] == '&') {
                token.type = tt_and;
                i += 2;
            } else {
                token.type = tt_bitwise_and;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '|': {
            if (i + 1 < line.size() && line[i + 1] == '|') {
                token.type = tt_or;
                i += 2;
            } else {
                token.type = tt_bitwise_or;
                ++i;
            }
            token.charEnd = i;
            break;
        }
        case '^': {
            token.type = tt_xor;
            ++i;
            token.charEnd = i;
            break;
        }
        case '~': {
            token.type = tt_bitwise_not;
            ++i;
            token.charEnd = i;
            break;
        }
        case '\'': {
            i++;
            if (i < line.size() && line[i] == '\\') {
                i += 2;
                if (i < line.size() && line[i] == '\'') {
                    token.type = tt_char;
                    token.str = (char*)s.arena.alloc(3);
                    token.str[0] = line[token.charStart + 1];
                    token.str[1] = line[token.charStart + 2];
                    token.str[2] = '\0';
                    ++i;
                    token.charEnd = i;
                    break;
                } else {
                    token.type = tt_error;
                    token.charEnd = i;
                    break;
                }
            }
            i++;
            if (i < line.size() && line[i] == '\'') {
                token.type = tt_char;
                token.str = (char*)s.arena.alloc(2);
                token.str[0] = line[token.charStart + 1];
                token.str[1] = '\0';
                ++i;
                token.charEnd = i;
                break;
            } else {
                token.type = tt_sptr;
                i--;
                token.charEnd = i;
                break;
            }
        }
        default: {
            token.type = tt_error;
            token.charEnd = i + 1;
            ++i;
            return token;
        }
    }

    return token;
}

Tokens::Tokens(std::string file)
    : line(0)
    , index(0)
    , arena(2048) {
    std::ifstream infile(file);
    filename = file;

    std::string lineContent;
    int lineNumber = 0;
    while (std::getline(infile, lineContent)) {
        lines.push_back(lineContent);  // Store the line for error reporting
        u64 i = 0;
        int inCommentStack = 0;
        tokensByLine.push_back(std::vector<Token>());
        while (i < lineContent.size()) {
            if (inCommentStack > 0) {
                if (lineContent[i] == '*' && i + 1 < lineContent.size() && lineContent[i + 1] == '/') {
                    inCommentStack--;
                    i += 2;  // skip the end of the multi-line comment
                    continue;
                }

                if (lineContent[i] == '/' && i + 1 < lineContent.size() && lineContent[i + 1] == '*') {
                    inCommentStack++;
                    i += 2;  // skip the start of the multi-line comment
                    continue;
                }
                i++;
                continue;
            }
            if (i < lineContent.size() && isspace(lineContent[i])) {
                ++i;
                continue;
            }
            if (lineContent[i] == '/' && i + 1 < lineContent.size() && lineContent[i + 1] == '/') {
                break;  // single-line comment
            }
            if (lineContent[i] == '/' && i + 1 < lineContent.size() && lineContent[i + 1] == '*') {
                i += 2;
                inCommentStack++;
                continue;
            }
            Token token = createToken(lineContent, i, *this);
            token.line = lineNumber;  // Set the line number for the token
            tokensByLine.back().push_back(token);
        }
        Token endLineToken;
        endLineToken.type = tt_endline;
        endLineToken.charStart = lineContent.size();
        endLineToken.charEnd = lineContent.size();
        endLineToken.str = nullptr;
        tokensByLine.back().push_back(endLineToken);
        lineNumber++;
    }
    Token endFileToken;
    endFileToken.type = tt_endfile;
    endFileToken.charStart = 0;
    endFileToken.charEnd = 0;
    endFileToken.str = nullptr;
    tokensByLine.push_back(std::vector<Token>());
    tokensByLine.back().push_back(endFileToken);

    infile.close();
}


Token Tokens::getToken() {
    if (line >= tokensByLine.size()) {
        return Token { tt_endfile, 0, 0, 0, nullptr };
    }
    if (index >= tokensByLine[line].size()) {
        line++;
        index = 0;
        return tokensByLine[line][index];
    }
    return tokensByLine[line][index++];
}

Token Tokens::peekToken() const {
    if (line >= tokensByLine.size()) {
        return Token { tt_endfile, 0, 0, 0, nullptr };
    }
    if (index >= tokensByLine[line].size()) {
        return Token { tt_endline, 0, 0, 0, nullptr };
    }
    return tokensByLine[line][index];
}

Token Tokens::peekToken(int amount) const {
    u64 line = this->line;
    u64 index = this->index;
    while (amount > 0) {
        index++;
        if (index >= tokensByLine[line].size()) {
            line++;
            index = 0;
        }
        if (line >= tokensByLine.size()) {
            return Token { tt_endfile, 0, 0, 0, nullptr };
        }
        amount--;
    }
    return tokensByLine[line][index];
}

Token Tokens::prevToken() const {
    u64 line = this->line;
    u64 index = this->index;
    if (line == 0 && index == 0) {
        return Token { tt_endfile, 0, 0, 0, nullptr };
    }
    if (index == 0) {
        line--;
        index = tokensByLine[line].size() - 1;
    } else {
        index--;
    }
    return tokensByLine[line][index];
}

Token Tokens::prevToken(int amount) const {
    u64 line = this->line;
    u64 index = this->index;
    while (amount > 0) {
        if (index == 0) {
            if (line == 0) {
                return Token { tt_endfile, 0, 0, 0, nullptr };
            }
            line--;
            index = tokensByLine[line].size() - 1;
        } else {
            index--;
        }
        amount--;
    }
    return tokensByLine[line][index];
}

Token Tokens::goBack(int amount) {
    while (amount > 0) {
        if (index == 0) {
            if (line == 0) {
                return Token { tt_endfile, 0, 0, 0, nullptr };
            }
            line--;
            index = tokensByLine[line].size() - 1;
        } else {
            index--;
        }
        amount--;
    }
    return tokensByLine[line][index];
}
Token Tokens::goBack() {
    if (line == 0 && index == 0) {
        return Token { tt_endfile, 0, 0, 0, nullptr };
    }
    if (index == 0) {
        line--;
        index = tokensByLine[line].size() - 1;
    } else {
        index--;
    }
    return tokensByLine[line][index];
}
