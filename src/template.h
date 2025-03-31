#pragma once
#include "utils.h"

class Type;

struct templateNames {
    vector<string> names;
    string getSimpleStr();
};

struct templateTypes {
    vector<Type> types;
    string getSimpleStr();
};

optional<templateNames> parseTemplateNames(bool logErrors = true);
optional<templateTypes> parseTemplateTypes(bool logErrors = true);