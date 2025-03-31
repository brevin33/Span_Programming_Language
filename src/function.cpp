#include "function.h"
#include "span.h"

optional<functionPrototype> prototypeFunction(int startPos) {
    functionPrototype proto;
    proto.startPosition = c.pos;
    int prevPos = c.pos;
    c.pos = startPos;
    optional<Type> returnType = parseType();
    if (!returnType.has_value()) {
        c.pos = prevPos;
        return nullopt;
    }
    proto.returnType = returnType.value();
    proto.methodType = parseType(false);
    if (proto.methodType.has_value()) {
        proto.paramNames.push_back("this");
        proto.paramTypes.push_back(proto.methodType.value());
        if (c.tokens[c.pos].type != tt_dot) {
            logError("Becauses ran into two types in a row. I expected a . to make a method", c.tokens[c.pos]);
            c.pos = prevPos;
            return nullopt;
        }
        c.pos++;
    }

    assert(c.tokens[c.pos].type == tt_id);
    proto.name = c.tokens[c.pos].getStr();
    c.pos++;

    // todo: figure out templates here more
    proto.templates = parseTemplateNames(false);

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

            optional<Type> paramType = parseType();
            if (!returnType.has_value()) {
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
        if (returnType.value().getName() != "void" && returnType.value().getName() != "i32") {
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

bool functionPrototype::isReal() {
    return !templates.has_value();
}
