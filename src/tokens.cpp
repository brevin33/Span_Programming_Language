#include "tokens.h"

string Token::getString() {
    assert(type == tt_string || type == tt_identifier);
    return string(str);
}

void Tokens::loadFile(string path) {
    ifstream file(path);
    if (!file.is_open()) {
        logError("Could not open file: " + path);
        return;
    }
    string line;
    u16 lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        for (u16 i = 0; i < line.length(); i++) {
            char c = line[i];
            char start = i;
            if (isspace(c)) continue;
            if (isdigit(c) || (i + 1 < line.length() && c == '.' && isdigit(line[i + 1]))) {
                stringstream ss;
                bool isReal = false;
                while (true) {
                    if (i >= line.length()) break;
                    if (!isdigit(line[i])) break;
                    ss << line[i];
                    if (line[i] == '.') {
                        isReal = true;
                        break;
                    }
                    i++;
                }
                if (isReal) {
                    i++;
                    while (true) {
                        if (i >= line.length()) break;
                        if (!isdigit(line[i])) break;
                        ss << line[i];
                        i++;
                    }
                    optional<f64> realValue = getReal(ss.str());
                    if (!realValue.has_value()) {
                        logError("Invalid real number: " + ss.str());
                        tokens.push_back(Token(tt_error, start, i, lineNumber, 0));
                        continue;
                    }
                    tokens.push_back(Token(tt_real, start, i, lineNumber, 0, realValue.value()));
                    continue;
                }
                optional<u64> intValue = getNumber(ss.str());
                if (!intValue.has_value()) {
                    logError("Invalid integer: " + ss.str());
                    tokens.push_back(Token(tt_error, start, i, lineNumber, 0));
                    continue;
                }
                tokens.push_back(Token(tt_integer, start, i, lineNumber, 0, intValue.value()));
                continue;
            } else if (isalpha(c) || c == '_') {
                stringstream ss;
                while (i < line.length() && (isalnum(line[i]) || line[i] == '_')) {
                    ss << line[i];
                    i++;
                }
                i--;
                tokens.push_back(Token(tt_identifier, start, i, lineNumber, 0, ss.str()));
                continue;
            } else if (c == '"') {
                stringstream ss;
                i++;
                while (i < line.length() && line[i] != '"') {
                    ss << line[i];
                    i++;
                }
                if (i >= line.length()) {
                    logError("Unterminated string literal at line " + to_string(lineNumber));
                    tokens.push_back(Token(tt_error, start, i, lineNumber, 0));
                    continue;
                }
                tokens.push_back(Token(tt_string, start, i, lineNumber, 0, ss.str()));
                continue;
            } else if (c == '/') {
                if (i + 1 < line.length() && line[i + 1] == '/') {
                    break;  // single-line comment
                }
            }
            switch (c) {
                default: {
                    logError("Unknown character: " + string(1, c));
                    tokens.push_back(Token(tt_error, start, i, lineNumber, 0));
                    continue;
                }
            }
        }
        tokens.push_back(Token(tt_endl, 0, 0, lineNumber, 0));
    }
    tokens.push_back(Token(tt_endf, 0, 0, lineNumber, 0));
}

void Tokens::print() {
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == tt_endl) {
            cout << endl;
            continue;
        }
        if (tokens[i].type == tt_endf) {
            cout << endl;
            cout << "End of file" << endl;
            cout << endl;
            continue;
        }
        cout << to_string(tokens[i]) << "  ";
    }
}
