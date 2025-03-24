#include "function.h"
#include "span.h"

optional<functionPrototype> prototypeFunction(int startPos) {
    functionPrototype proto;
    int prevPos = c.pos;
    c.pos = startPos;
    optional<Type> type = typeFromTokens();
    if (!type.has_value()) {
        c.pos = prevPos;
        return nullopt;
    }
    proto.returnType = type.value();

    assert(c.tokens[c.pos].type == tt_id);
    proto.name = c.tokens[c.pos].getStr();
    c.pos++;

    assert(c.tokens[c.pos].type == tt_lpar);
    c.pos++;

    proto.variadicArgs = false;
    if (c.tokens[c.pos].type != tt_rpar) {
        while (true) {

            if (c.tokens[c.pos].type == tt_elips) {
                c.pos++;
                if (c.tokens[c.pos].type != tt_rpar) {
                    logError("Expected a )", c.tokens[c.pos]);
                    c.pos = prevPos;
                    return nullopt;
                }
                proto.variadicArgs = true;
                break;
            }

            optional<Type> paramType = typeFromTokens();
            if (!type.has_value()) {
                c.pos = prevPos;
                return nullopt;
            }

            if (c.tokens[c.pos].type != tt_id) {
                logError("Expected parameter name", c.tokens[c.pos]);
                c.pos = prevPos;
                return nullopt;
            }
            string paramName = c.tokens[c.pos].getStr();
            c.pos++;

            proto.paramTypes.push_back(paramType.value());
            proto.paramNames.push_back(paramName);

            if (c.tokens[c.pos].type == tt_rpar) break;
            if (c.tokens[c.pos].type == tt_com) {
                c.pos++;
                continue;
            }
            logError("Expected , or )", c.tokens[c.pos]);
            c.pos = prevPos;
            return nullopt;
        }
    }
    c.pos++;

    if (proto.name == "main") {
        if (type.value().getName() != "void" && type.value().getName() != "i32") {
            logError("return type of main must be i32 or void", c.tokens[c.pos], true);
            c.pos = prevPos;
            return nullopt;
        }
        if (c.activeModule->hasMain) {
            logError("already has a main function. can'c.t have multiple main", c.tokens[c.pos], true);
            c.pos = prevPos;
            return nullopt;
        }
        c.activeModule->hasMain = true;
    }
    c.pos = prevPos;
    return proto;
}
