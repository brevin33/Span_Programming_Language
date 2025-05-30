#include "parser/project.h"
#include "parser.h"
#include "parser/tokens.h"
#include "parser/type.h"
#include <algorithm>
#include <filesystem>
#include <functional>

using namespace std;
using namespace std::filesystem;

Project::Project(string folder)
    : folder(folder) {
    if (is_directory(folder)) {
        for (const auto& entry : directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".span") {
                this->tokens.emplace_back(entry.path().string());
            }
        }
    } else {
        logError("Project folder does not exist or is not a directory: " + folder);
        throw runtime_error("Project folder does not exist or is not a directory: " + folder);
    }


    if (this->tokens.empty()) {
        logError("No .span files found in the project folder: " + folder);
        throw runtime_error("No .span files found in the project folder: " + folder);
    }

    for (u64 i = 0; i < this->tokens.size(); i++) {
        globalParse(i);
    }

    for (u64 i = 0; i < functionStarts.size(); i++) {
        ProjectTokenPos& pos = functionStarts[i];
        protoTypeFunctions(pos);
    }
}


TypeId Project::parseType(Tokens& tokens, bool& err, bool errorOnFail) {
    TokenPos startPos = tokens.getPosition();
    Token token = tokens.getToken();
    if (token.type != tt_id) {
        if (errorOnFail) {
            logError("Expected type identifier, got: " + to_string(token), token, tokens);
        }
        err = true;
        tokens.setPosition(startPos);
        return TypeId { 0 };  // Invalid TypeId
    }
    string baseName = token.getString();
    TypeId typeId = getTypeFromName(baseName, err);

    //TODO: pickup from here
}


// should reset tokens to the postion they started at
void Project::protoTypeFunctions(ProjectTokenPos& pos) {
    Tokens& tokens = this->tokens[pos.fileIndex];
    TokenPos startPos = tokens.getPosition();
    tokens.setPosition(pos.pos);
    Token token = tokens.getToken();

    tokens.setPosition(startPos);
}

bool looksLikeScope(Tokens& tokens) {
    TokenPos start = tokens.getPosition();
    // skip any scope
    int braceCount = 1;
    tokens.getToken();  // consume the opening brace
    while (braceCount > 0) {
        Token nextToken = tokens.getToken();
        if (nextToken.type == tt_lbrace) {
            braceCount++;
        } else if (nextToken.type == tt_rbrace) {
            braceCount--;
        } else if (nextToken.type == tt_endfile) {
            tokens.setPosition(start);
            return false;  // invalid scope
        }
    }
    tokens.getToken();
    return true;
}

bool looksLikeTemplateArgs(Tokens& tokens) {
    TokenPos start = tokens.getPosition();
    if (tokens.getToken().type != tt_less_than) {
        tokens.setPosition(start);
        return false;
    }
    bool atLeastOneType = false;
    while (true) {
        Token token = tokens.getToken();
        if (token.type == tt_greater_than) {
            break;
        } else if (token.type == tt_id) {
            atLeastOneType = true;  // found a type
            Token nextToken = tokens.peekToken();
            if (nextToken.type == tt_colon) {
                // can specify type of template argument
                if (nextToken.type != tt_id) {
                    tokens.setPosition(start);
                    return false;  // invalid type
                }
            }
            continue;  // valid type
        } else {
            tokens.setPosition(start);
            return false;  // invalid type
        }
    }
    if (!atLeastOneType) {
        tokens.setPosition(start);
        return false;  // no types found
    }

    return true;
}

// in the event of false must resets the token position
bool looksLikeType(Tokens& tokens) {
    TokenPos start = tokens.getPosition();
    Token token = tokens.getToken();
    if (token.type != tt_id) {
        tokens.setPosition(start);
        return false;
    }
    looksLikeTemplateArgs(tokens);
    while (true) {
        Token token = tokens.getToken();
        if (token.type == tt_multiply || token.type == tt_bitwise_and || token.type == tt_lbracket) {
            if (token.type == tt_lbracket) {
                token = tokens.getToken();
                if (token.type == tt_rbracket) {
                    continue;
                }
                while (true) {
                    if (token.type == tt_rbracket) {
                        break;
                    } else if (token.type == tt_endfile || token.type == tt_endline) {
                        tokens.setPosition(start);
                        return false;
                    }
                }
            }
            continue;
        } else {
            tokens.goBack();
            return true;
        }
    }
}

bool looksLikeFunction(Tokens& tokens) {
    TokenPos start = tokens.getPosition();
    if (!looksLikeType(tokens)) {
        tokens.setPosition(start);
        return false;
    }
    Token token = tokens.getToken();
    if (token.type != tt_id) {
        tokens.setPosition(start);
        return false;
    }
    looksLikeTemplateArgs(tokens);

    token = tokens.getToken();
    if (token.type != tt_lparen) {
        tokens.setPosition(start);
        return false;
    }
    while (true) {
        token = tokens.getToken();
        if (token.type == tt_rparen) {
            break;
        } else if (token.type == tt_endfile) {
            tokens.setPosition(start);
            return false;
        }
    }

    if (!looksLikeScope(tokens)) {
        tokens.setPosition(start);
        return false;  // invalid scope
    }
    Token nextToken = tokens.peekToken();

    return true;
}

bool looksLikeGlobalVariable(Tokens& token) {
    //TODO
    return false;
}

bool looksLikeStruct(Tokens& token) {
    // TODO
    return false;
}

void Project::globalParse(u64 tokensIndex) {
    Tokens& tokens = this->tokens[tokensIndex];
    while (true) {
        TokenPos start = tokens.getPosition();
        Token token = tokens.peekToken();
        // find start pos for all global item
        // aka variable, function, types
        switch (token.type) {
            case tt_id: {
                if (looksLikeFunction(tokens)) {
                    functionStarts.push_back({ start, tokensIndex });
                    break;
                } else if (looksLikeGlobalVariable(tokens)) {
                    globalVariableStarts.push_back({ start, tokensIndex });
                    break;
                } else {
                    logError("Global variable or function start is defined incorrectly: " + to_string(token), token, tokens);
                    tokens.getToken();  // consume the token
                    break;
                }
            }
            case tt_struct: {
                if (looksLikeStruct(tokens)) {
                    structStarts.push_back({ start, tokensIndex });
                    break;
                } else {
                    logError("Struct start is defined incorrectly: " + to_string(token), token, tokens);
                    tokens.getToken();  // consume the token
                    break;
                }
            }
            case tt_endline: {
                tokens.getToken();
                break;
            }
            case tt_lbrace: {
                looksLikeScope(tokens);
                break;
            }
            case tt_endfile: {
                return;  // End of file, exit the loop
            }
            default: {
                logError("Unexpected token type in globalParse: " + to_string(token.type), token, tokens);
                tokens.getToken();  // consume the token
                break;
            }
        }
    }
}
