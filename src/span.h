#pragma once
#include "utils.h"
#include "function.h"
#include "value.h"
#include "type.h"
#include "token.h"
#include "module.h"
#include "variable.h"
#include "scope.h"

extern LLVMContextRef context;
extern LLVMBuilderRef builder;
extern Module* baseModule;
extern unordered_map<string, vector<Type>> nameToType;
extern unordered_map<string, vector<Function>> nameToFunction;

void compile(const std::string& dir);
