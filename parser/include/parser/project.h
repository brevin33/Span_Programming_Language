#pragma once
#include "parser/tokens.h"
#include "parser/type.h"
#include <string>


struct ProjectTokenPos {
    TokenPos pos;
    u64 fileIndex;
};

struct Project {
    Project(std::string folder);
    ~Project() {
    }
    void globalParse(u64 tokensIndex);
    void protoTypeFunctions(ProjectTokenPos& tokensPos);
    TypeId parseType(Tokens& tokens, bool& err, bool errorOnFail = false);
    std::string folder;
    std::vector<Tokens> tokens;

    // global starts
    std::vector<ProjectTokenPos> functionStarts;
    std::vector<ProjectTokenPos> globalVariableStarts;
    std::vector<ProjectTokenPos> structStarts;
};
