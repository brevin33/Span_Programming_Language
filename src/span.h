#pragma once
#include "utils.h"
#include "function.h"
#include "value.h"
#include "type.h"
#include "token.h"
#include "module.h"
#include "variable.h"
#include "scope.h"
#include "lld/Common/Driver.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Target.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

extern LLVMContextRef context;
extern LLVMBuilderRef builder;
extern Module* baseModule;
extern unordered_map<string, vector<Type>> nameToType;
extern unordered_map<string, vector<Function>> nameToFunction;
extern Module* activeModule;

void compile(const std::string& dir);
