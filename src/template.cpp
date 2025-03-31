#include "template.h"
#include "span.h"
#include "types.h"

optional<templateNames> parseTemplateNames(bool logErrors) {
    int s = c.pos;
    if (c.tokens[c.pos].type != tt_le) {
        if (logErrors) logError("expected a <", c.tokens[c.pos]);
        return nullopt;
    }
    c.pos++;
    templateNames tn;
    while (true) {
        if (c.tokens[c.pos].type == tt_gr) break;
        if (c.tokens[c.pos].type != tt_id) {
            if (logErrors) logError("expected a template name or >", c.tokens[c.pos]);
            c.pos = s;
            return nullopt;
        }
        tn.names.push_back(c.tokens[c.pos].getStr());
        c.pos++;
    }
    c.pos++;
    return tn;
}

optional<templateTypes> parseTemplateTypes(bool logErrors) {
    int s = c.pos;
    if (c.tokens[c.pos].type != tt_le) {
        if (logErrors) logError("expected a <", c.tokens[c.pos]);
        return nullopt;
    }
    c.pos++;
    templateTypes tt;
    while (true) {
        if (c.tokens[c.pos].type == tt_gr) break;
        optional<Type> type = parseType(logErrors);
        if (!type.has_value()) {
            c.pos = s;
            return nullopt;
        }
        tt.types.push_back(type.value());
    }
    c.pos++;
    return tt;
}

string templateNames::getSimpleStr() {
    string str = "<";
    for (int i = 0; i < names.size(); i++) {
        str += "T" + to_string(i) + " ";
    }
    str += '>';
    return str;
}

string templateTypes::getSimpleStr() {
    string str = "<";
    for (int i = 0; i < types.size(); i++) {
        str += "T" + to_string(i) + " ";
    }
    str += '>';
    return str;
}
