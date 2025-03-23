#include "module.h"
#include "span.h"

Module::Module(const string& dir)
    : tokens(context.tokens) {
    this->dir = dir;
    llvmModule = LLVMModuleCreateWithName(dir.c_str());
    context.activeModule = this;

    int startFile = context.textByFileByLine.size();
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != ".span") continue;
        context.files.push_back(path.string());
        context.textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
    }
    this->tokensStart = getTokens(context.textByFileByLine, startFile);
    this->tokensEnd = context.tokens.size();
    int i = tokensStart;
}

bool Module::looksLikeType() {
    int start = t;
    if (tokens[t].type != tt_id) {
        t = start;
        return false;
    }
    t++;
    while (true) {
        switch (tokens[t].type) {
            case tt_mul: {
                t++;
                break;
            }
            case tt_and: {
                t++;
                break;
            }
            case tt_lbar: {
                t++;
                if (tokens[t].type != tt_int) {
                    t = start;
                    return false;
                }
                t++;
                if (tokens[t].type != tt_rbar) {
                    t = start;
                    return false;
                }
                t++;
                break;
            }
            case tt_bitor: {
                t++;
                if (!looksLikeType()) {
                    t = start;
                    return false;
                }
                return true;
            }
            case tt_com: {
                t++;
                if (!looksLikeType()) {
                    t = start;
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

bool Module::looksLikeFunction() {
    int start = t;
    if (!looksLikeType()) return false;
    int typeEnd = t;
    if (looksLikeType()) {
        if (tokens[t].type != tt_dot) {
            t = typeEnd;
        } else {
            t++;
        }
    }
    if (tokens[t].type != tt_id) {
        t = start;
        return false;
    }
    t++;
    if (tokens[t].type != tt_lpar) {
        t = start;
        return false;
    }
    while (true) {
        t++;
        if (tokens[t].type == tt_rpar) break;
    }
    t++;
    if (tokens[t].type == tt_endl) {
        return true;
    }
    if (tokens[t].type != tt_lcur) {
        t = start;
        return false;
    }
    int curStack = 1;
    Token curStart = tokens[t];
    while (curStack != 0) {
        t++;
        if (tokens[t].type == tt_eot) {
            t = start;
            return false;
        }
        if (tokens[t].type == tt_eof) {
            t = start;
            return false;
        }
        if (tokens[t].type == tt_rcur) curStack--;
        if (tokens[t].type == tt_lcur) curStack++;
    }
    t++;
    if (tokens[t].type == tt_endl) return true;
    if (tokens[t].type == tt_eof) return true;
    t = start;
    return false;
}

void Module::findStarts() {
    while (true) {
        int s = tokensStart;
        if (looksLikeFunction()) {
            functionStarts.push_back(s);
        } else if (tokens[t].type == tt_endl) {
            t++;
            continue;
        } else if (tokens[t].type == tt_eof) {
            t++;
            continue;
        } else if (tokens[t].type == tt_eot) {
            return;
        } else {
            logError("Don't know what this top level line is", tokens[t], true);
            while (true) {
                if (tokens[t].type == tt_endl) break;
                if (tokens[t].type == tt_eof) break;
                if (tokens[t].type == tt_lcur) {
                    Token curStart = tokens[t];
                    int curStack = 1;
                    while (curStack != 0) {
                        t++;
                        if (tokens[t].type == tt_eot) return;
                        if (tokens[t].type == tt_eof) break;
                        if (tokens[t].type == tt_rcur) curStack--;
                        if (tokens[t].type == tt_lcur) curStack++;
                    }
                    if (tokens[t].type == tt_eof) {
                        logError("No closing bracket", curStart);
                        break;
                    }
                    t++;
                    if (tokens[t].type == tt_endl) break;
                    if (tokens[t].type == tt_eof) break;
                    if (tokens[t].type == tt_eot) return;
                    logError("Right brackets should be on there own line", tokens[t]);
                }
                t++;
            }
        }
    }
}
