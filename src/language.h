#pragma once
#include "utils.h"
#include "module.h"


class SpanProgram {

public:
    SpanProgram(string dir);

    void compile();

    Module module;
};
