#pragma once
#include "utils.h"
#include "value.h"

struct Module;

class Variable {
public:
    Variable();
    Variable(string& name, Type& type, Module* module);
    Variable(string& name, Type& type, Value& val, Module* module);

    void store(const Value& value);


public:
    string name;
    Value value;
    Module* module;

private:
private:
};