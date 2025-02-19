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

LLVMValueRef createFormatString(LLVMModuleRef module, const char* str) {
    // Create a global constant for the format string
    LLVMValueRef formatStr = LLVMAddGlobal(module, LLVMArrayType(LLVMInt8Type(), strlen(str) + 1), "fmt");
    LLVMSetInitializer(formatStr, LLVMConstString(str, strlen(str) + 1, true));
    LLVMSetGlobalConstant(formatStr, true);
    LLVMSetLinkage(formatStr, LLVMPrivateLinkage);

    // Get pointer to the start of the string
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef indices[] = { zero, zero };
    return LLVMBuildInBoundsGEP2(builder, LLVMTypeOf(formatStr), formatStr, indices, 2, "fmtPtr");
}

void setupBasicTypes() {
    // number types
    for (int i = 1; i <= 512; i++) {
        string uname = "u" + to_string(i);
        string iname = "i" + to_string(i);
        nameToType[uname].push_back(Type(LLVMIntType(i), uname, baseModule));
        nameToType[iname].push_back(Type(LLVMIntType(i), iname, baseModule));
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


    // print function
    LLVMTypeRef printfArgTypes[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef printfType = LLVMFunctionType(LLVMInt32Type(), printfArgTypes, 1, true);
    LLVMValueRef printfFunc = LLVMAddFunction(baseModule->llvmModule, "printf", printfType);

    LLVMTypeRef paramTypes[] = { LLVMInt64Type() };
    LLVMTypeRef printI64Type = LLVMFunctionType(LLVMVoidType(), paramTypes, 1, false);
    LLVMValueRef printI64Func = LLVMAddFunction(baseModule->llvmModule, "print_i64", printI64Type);

    Function intPrint;
    intPrint.llvmType = printI64Type;
    intPrint.llvmValue = printI64Func;
    intPrint.name = "print";
    intPrint.paramNames = { "a" };
    bool err;
    intPrint.paramTypes = { nameToType["i64"].front() };
    intPrint.returnType = nameToType["void"].front();
    nameToFunction["print"].push_back(Function(intPrint.returnType, intPrint.name, intPrint.paramTypes, intPrint.paramNames, baseModule));

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(printI64Func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef i64Value = LLVMGetParam(printI64Func, 0);
    // Create a global constant for the format string
    char* str = "%d\n";
    LLVMValueRef formatStr = LLVMAddGlobal(baseModule->llvmModule, LLVMArrayType(LLVMInt8Type(), strlen(str) + 1), "fmt");
    LLVMSetInitializer(formatStr, LLVMConstString(str, strlen(str) + 1, true));
    LLVMSetGlobalConstant(formatStr, true);
    LLVMSetLinkage(formatStr, LLVMPrivateLinkage);

    // Get pointer to the start of the string
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef indices[] = { zero, zero };
    LLVMValueRef formatStr2 = LLVMBuildInBoundsGEP2(builder, LLVMTypeOf(formatStr), formatStr, indices, 2, "fmtPtr");

    LLVMValueRef printfArgs[] = { formatStr2, i64Value };
    LLVMBuildCall2(builder, printfType, printfFunc, printfArgs, 2, "calltmp");
    LLVMBuildRetVoid(builder);
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

    baseModule->printResult();

    module.printResult();
}
