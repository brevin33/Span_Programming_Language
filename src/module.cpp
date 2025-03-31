#include "module.h"
#include "span.h"

Module::Module(const string& dir) {
    this->dir = dir;
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    c.activeModule = this;

    int startFile = c.textByFileByLine.size();
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        c.files.push_back(path.string());
        c.textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
    }
    this->tokensStart = getTokens(c.textByFileByLine, startFile);
    this->tokensEnd = c.tokens.size();
    c.pos = tokensStart;

    findStarts();
    for (int i = 0; i < functionStarts.size(); i++) {
        optional<functionPrototype> proto = prototypeFunction(functionStarts[i]);
        if (proto.has_value()) {
            functionPrototypes[proto.value().name] = proto.value();
        }
    }
}



bool looksLikeType() {
    int start = c.pos;
    if (c.tokens[c.pos].type != tt_id) {
        c.pos = start;
        return false;
    }
    looksLikeTemplateNames();
    c.pos++;
    while (true) {
        switch (c.tokens[c.pos].type) {
            case tt_mul: {
                c.pos++;
                break;
            }
            case tt_and: {
                c.pos++;
                break;
            }
            case tt_lbar: {
                c.pos++;
                if (c.tokens[c.pos].type != tt_int) {
                    c.pos = start;
                    return false;
                }
                c.pos++;
                if (c.tokens[c.pos].type != tt_rbar) {
                    c.pos = start;
                    return false;
                }
                c.pos++;
                break;
            }
            case tt_bitor: {
                c.pos++;
                if (!looksLikeType()) {
                    c.pos = start;
                    return false;
                }
                return true;
            }
            case tt_com: {
                c.pos++;
                if (!looksLikeType()) {
                    c.pos = start;
                    return false;
                }
                return true;
            }
            default: {
                return true;
                break;
            }
        }
    }
}

bool looksLikeTemplateTypes() {
    int start = c.pos;
    if (c.tokens[c.pos].type != tt_le) {
        c.pos = start;
        return false;
    }
    c.pos++;
    while (true) {
        if (c.tokens[c.pos].type == tt_gr) break;
        if (!looksLikeType()) {
            c.pos = start;
            return false;
        }
    }
    return true;
}

bool looksLikeTemplateNames() {
    int start = c.pos;
    if (c.tokens[c.pos].type != tt_le) {
        c.pos = start;
        return false;
    }
    c.pos++;
    while (true) {
        if (c.tokens[c.pos].type == tt_gr) break;
        if (c.tokens[c.pos].type != tt_id) {
            c.pos = start;
            return false;
        }
        c.pos++;
    }
    return true;
}


bool looksLikeFunction() {
    int start = c.pos;
    if (!looksLikeType()) return false;
    int typeEnd = c.pos;
    if (looksLikeType()) {
        if (c.tokens[c.pos].type != tt_dot) {
            c.pos = typeEnd;
        } else {
            c.pos++;
        }
    }
    if (c.tokens[c.pos].type != tt_id) {
        c.pos = start;
        return false;
    }
    c.pos++;
    looksLikeTemplateNames();
    if (c.tokens[c.pos].type != tt_lpar) {
        c.pos = start;
        return false;
    }
    while (true) {
        c.pos++;
        if (c.tokens[c.pos].type == tt_rpar) break;
    }
    c.pos++;
    if (c.tokens[c.pos].type == tt_endl) {
        return true;
    }
    if (c.tokens[c.pos].type != tt_lcur) {
        c.pos = start;
        return false;
    }
    int curStack = 1;
    Token curStart = c.tokens[c.pos];
    while (curStack != 0) {
        c.pos++;
        if (c.tokens[c.pos].type == tt_eot) {
            c.pos = start;
            return false;
        }
        if (c.tokens[c.pos].type == tt_eof) {
            c.pos = start;
            return false;
        }
        if (c.tokens[c.pos].type == tt_rcur) curStack--;
        if (c.tokens[c.pos].type == tt_lcur) curStack++;
    }
    c.pos++;
    if (c.tokens[c.pos].type == tt_endl) return true;
    if (c.tokens[c.pos].type == tt_eof) return true;
    c.pos = start;
    return false;
}


void Module::findStarts() {
    while (true) {
        int s = c.pos;
        if (looksLikeFunction()) {
            functionStarts.push_back(s);
        } else if (c.tokens[c.pos].type == tt_endl) {
            c.pos++;
            continue;
        } else if (c.tokens[c.pos].type == tt_eof) {
            c.pos++;
            continue;
        } else if (c.tokens[c.pos].type == tt_eot) {
            return;
        } else {
            logError("Don't know what this top level line is", c.tokens[c.pos], true);
            while (true) {
                if (c.tokens[c.pos].type == tt_endl) break;
                if (c.tokens[c.pos].type == tt_eof) break;
                if (c.tokens[c.pos].type == tt_lcur) {
                    Token curStart = c.tokens[c.pos];
                    int curStack = 1;
                    while (curStack != 0) {
                        c.pos++;
                        if (c.tokens[c.pos].type == tt_eot) return;
                        if (c.tokens[c.pos].type == tt_eof) break;
                        if (c.tokens[c.pos].type == tt_rcur) curStack--;
                        if (c.tokens[c.pos].type == tt_lcur) curStack++;
                    }
                    if (c.tokens[c.pos].type == tt_eof) {
                        logError("No closing bracket", curStart);
                        break;
                    }
                    c.pos++;
                    if (c.tokens[c.pos].type == tt_endl) break;
                    if (c.tokens[c.pos].type == tt_eof) break;
                    if (c.tokens[c.pos].type == tt_eot) return;
                    logError("Right brackets should be on there own line", c.tokens[c.pos]);
                }
                c.pos++;
            }
        }
    }
}
