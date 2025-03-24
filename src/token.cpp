#include "token.h"
#include "span.h"

int getTokens(const vector<vector<string>>& textByFileByLine, int startFile) {
    int startOfTokens = c.tokens.size();
    vector<Token>& tokens = c.tokens;
    for (u16 file = startFile; file < textByFileByLine.size(); file++) {
        for (u16 lineNum = 0; lineNum < textByFileByLine[file].size(); lineNum++) {
            const string& line = textByFileByLine[file][lineNum];
            for (u16 c = 0; c < line.size(); c++) {
                while (c < line.size() && (isspace(line[c]) || line[c] == '\0'))
                    c++;
                if (c >= line.size()) break;
                Token token;
                token.schar = c;
                token.line = lineNum;
                token.file = file;

                switch (line[c]) {
                    case '_':
                    CASELETTER: {
                        stringstream ss;
                        bool done = false;
                        while (c < line.size() && !done) {
                            switch (line[c]) {
                                case '_':
                                CASELETTER:
                                CASENUMBER: {
                                    ss << line[c];
                                    c++;
                                    break;
                                }
                                default: {
                                    done = true;
                                    break;
                                }
                            }
                        }
                        c--;
                        token.echar = c;
                        string str = ss.str();
                        if (str == "return") {
                            token.type = tt_ret;
                            break;
                        }
                        if (str == "if") {
                            token.type = tt_if;
                            break;
                        }
                        if (str == "in") {
                            token.type = tt_in;
                            break;
                        }
                        if (str == "else") {
                            token.type = tt_else;
                            break;
                        }
                        if (str == "switch") {
                            token.type = tt_switch;
                            break;
                        }
                        if (str == "switch") {
                            token.type = tt_case;
                            break;
                        }
                        if (str == "for") {
                            token.type = tt_for;
                            break;
                        }
                        if (str == "case") {
                            token.type = tt_case;
                            break;
                        }
                        if (str == "while") {
                            token.type = tt_while;
                            break;
                        }
                        if (str == "break") {
                            token.type = tt_break;
                            break;
                        }
                        if (str == "continue") {
                            token.type = tt_continue;
                            break;
                        }
                        if (str == "continue") {
                            token.type = tt_struct;
                            break;
                        }
                        if (str == "as") {
                            token.type = tt_as;
                            break;
                        }
                        if (str == "struct") {
                            token.type = tt_struct;
                            break;
                        }
                        if (str == "to") {
                            token.type = tt_to;
                            break;
                        }
                        if (str == "enum") {
                            token.type = tt_enum;
                            break;
                        }
                        if (str == "default") {
                            token.type = tt_default;
                            break;
                        }
                        if (str == "loop") {
                            // loop basiclly working like a macro for while true
                            Token token2;
                            token2.echar = token.echar;
                            token2.schar = token.schar;
                            token2.file = token.file;
                            token2.line = token.line;
                            token2.data.str = new string;
                            *token2.data.str = "while";
                            token2.type = tt_id;

                            token.type = tt_int;
                            token.data.uint = 1;
                            tokens.push_back(token2);
                            break;
                        }
                        if (str == "true") {
                            token.type == tt_int;
                            token.data.uint = 1;
                            break;
                        }
                        if (str == "false") {
                            token.type == tt_int;
                            token.data.uint = 0;
                            break;
                        }
                        // Add more keywords here
                        token.data.str = new string;
                        *token.data.str = str;
                        token.type = tt_id;
                        break;
                    }
                    case '"': {
                        c++;
                        stringstream ss;
                        bool done = false;
                        while (c < line.size() && !done) {
                            switch (line[c]) {
                                case '"': {
                                    done = true;
                                    break;
                                }
                                case '\\': {
                                    if (c + 1 < line.size() && line[c + 1] == 'n') {
                                        c += 2;
                                        ss << '\n';
                                        break;
                                    }
                                    if (c + 1 < line.size() && line[c + 1] == '\\') {
                                        c += 2;
                                        ss << '\\';
                                        break;
                                    }
                                    if (c + 1 < line.size() && line[c + 1] == '0') {
                                        c += 2;
                                        ss << '\0';
                                        break;
                                    }
                                    if (c + 1 < line.size() && line[c + 1] == 'c.t') {
                                        c += 2;
                                        ss << '\c.t';
                                        break;
                                    }
                                }
                                default: {
                                    ss << line[c];
                                    c++;
                                    break;
                                }
                            }
                        }
                        if (!done) {
                            token.echar = c;
                            token.type = tt_err;
                            break;
                        }
                        token.echar = c;
                        token.data.str = new string;
                        *token.data.str = ss.str();
                        token.type = tt_str;
                        break;
                    }
                    case '.': {
                        if (c + 1 < line.size() && line[c + 1] == '.') {
                            if (c + 2 < line.size() && line[c + 2] == '.') {
                                c += 2;
                                token.echar = c;
                                token.type = tt_elips;
                                break;
                            }
                            if (c + 2 < line.size() && line[c + 2] == '=') {
                                c += 2;
                                token.echar = c;
                                token.type = tt_elipseq;
                                break;
                            }
                        }
                        token.echar = c;
                        token.type = tt_dot;
                        break;
                    }
                    CASENUMBER: {
                        bool isFloat = false;
                        stringstream ss;
                        while (c < line.size() && (isdigit(line[c]) || line[c] == '.')) {
                            ss << line[c];
                            if (line[c] == '.') isFloat = true;
                            c++;
                        }
                        if (line[c - 1] == '.') {
                            isFloat = false;
                            c--;
                        }
                        c--;
                        token.echar = c;
                        if (token.schar == token.echar && line[c] == '.') {
                            token.type = tt_err;
                            break;
                        }
                        token.type = isFloat ? tt_float : tt_int;
                        if (isFloat) {
                            token.data.dec = std::stod(ss.str());
                        } else {
                            token.data.uint = std::stoull(ss.str());
                        }
                        break;
                    }
                    case '=': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_eqeq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_eq;
                        token.echar = c;
                        break;
                    }
                    case '|': {
                        if (c + 1 < line.size() && line[c + 1] == '|') {
                            c++;
                            token.type = tt_or;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_bitor;
                        token.echar = c;
                        break;
                    }
                    case '&': {
                        if (c + 1 < line.size() && line[c + 1] == '&') {
                            c++;
                            token.type = tt_and;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_bitand;
                        token.echar = c;
                        break;
                    }
                    case '<': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_leeq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_le;
                        token.echar = c;
                        break;
                    }
                    case '>': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_greq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_gr;
                        token.echar = c;
                        break;
                    }
                    case '!': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_neq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_ex;
                        token.echar = c;
                        break;
                    }
                    case '+': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_addeq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_add;
                        token.echar = c;
                        break;
                    }
                    case ';': {
                        token.type = tt_semi;
                        token.echar = c;
                        break;
                    }
                    case '-': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_subeq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_sub;
                        token.echar = c;
                        break;
                    }
                    case '*': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_muleq;
                            token.echar = c;
                            break;
                        }
                        token.type = tt_mul;
                        token.echar = c;
                        break;
                    }
                    case '/': {
                        if (c + 1 < line.size() && line[c + 1] == '=') {
                            c++;
                            token.type = tt_diveq;
                            token.echar = c;
                            break;
                        }
                        if (c + 1 < line.size() && line[c + 1] == '/') {
                            c = line.size();
                            continue;
                        }
                        token.type = tt_div;
                        token.echar = c;
                        break;
                    }
                    case '(': {
                        token.type = tt_lpar;
                        token.echar = c;
                        break;
                    }
                    case ')': {
                        token.type = tt_rpar;
                        token.echar = c;
                        break;
                    }
                    case '{': {
                        token.type = tt_lcur;
                        token.echar = c;
                        break;
                    }
                    case '}': {
                        token.type = tt_rcur;
                        token.echar = c;
                        break;
                    }
                    case '[': {
                        token.type = tt_lbar;
                        token.echar = c;
                        break;
                    }
                    case ']': {
                        token.type = tt_rbar;
                        token.echar = c;
                        break;
                    }
                    case '^': {
                        token.type = tt_car;
                        token.echar = c;
                        break;
                    }
                    case ',': {
                        token.type = tt_com;
                        token.echar = c;
                        break;
                    }
                    default: {
                        token.echar = c;
                        token.type = tt_err;
                        break;
                    }
                }
                tokens.push_back(token);
            }
            Token eol;
            eol.type = tt_endl;
            eol.schar = line.size();
            eol.echar = line.size();
            eol.line = lineNum;
            eol.file = file;
            tokens.push_back(eol);
        }
        Token eof;
        eof.type = tt_eof;
        eof.schar = UINT16_MAX;
        eof.echar = UINT16_MAX;
        eof.line = UINT16_MAX;
        eof.file = file;
        tokens.push_back(eof);
    }
    Token eot;
    eot.type = tt_eot;
    eot.schar = UINT16_MAX;
    eot.echar = UINT16_MAX;
    eot.line = UINT16_MAX;
    eot.file = UINT8_MAX;
    tokens.push_back(eot);
    return startOfTokens;
}
