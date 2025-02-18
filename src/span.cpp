#include "span.h"
#include <filesystem>
#include <fstream>
#include <assert.h>


LLVMContextRef context;
LLVMBuilderRef builder;
Module* baseModule;
unordered_map<string, vector<Type>> nameToType;
unordered_map<string, vector<Function>> nameToFunction;
Module* activeModule;

void setupBasicTypes() {
    for (int i = 0; i < 512; i++) {
        string uname = "u" + to_string(i);
        string iname = "i" + to_string(i);
        nameToType[uname].push_back(Type(LLVMIntType(64), uname, baseModule));
        nameToType[iname].push_back(Type(LLVMIntType(64), iname, baseModule));
    }
    nameToType["void"].push_back(Type(LLVMVoidType(), "void", baseModule));

    nameToType["int"].push_back(Type(LLVMIntType(32), "i32", baseModule));
    nameToType["uint"].push_back(Type(LLVMIntType(32), "u32", baseModule));

    nameToType["f16"].push_back(Type(LLVMHalfType(), "f16", baseModule));
    nameToType["f32"].push_back(Type(LLVMFloatType(), "f32", baseModule));
    nameToType["f64"].push_back(Type(LLVMDoubleType(), "f64", baseModule));

    nameToType["half"].push_back(Type(LLVMHalfType(), "f16", baseModule));
    nameToType["float"].push_back(Type(LLVMFloatType(), "f32", baseModule));
    nameToType["double"].push_back(Type(LLVMDoubleType(), "f64", baseModule));
}

void compile(const std::string& dir) {
    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
    baseModule = new Module("base");
    setupBasicTypes();

    Module module(dir);
    module.loadTokens();
    module.findStarts();
    module.setupTypesAndFunctions();



    module.printResult();


    delete baseModule;
}
