#include "compiler.h"
#include "parser/arena.h"
#include "llvm-c/Core.h"

LLVMTypeRef* typeIdToLLVM;

LLVMValueRef* functionIdtoLLVMFunction;

LLVMModuleRef* projectIdToLLVMModule;

LLVMTypeRef getLLVMType(typeId typeId) {
    Type* type = getTypeFromId(typeId);
    switch (type->kind) {
        case tk_float: {
            switch (type->numberSize) {
                case 2:
                    return LLVMHalfType();
                case 4:
                    return LLVMFloatType();
                case 8:
                    return LLVMDoubleType();
            }
            case tk_uint:
            case tk_int: {
                u64 numSize = type->numberSize;
                return LLVMIntType(numSize);
            }
            case tk_ref:
            case tk_pointer: {
                return LLVMPointerType(LLVMVoidType(), 0);
            }
            case tk_array: {
                return LLVMArrayType(getLLVMType(type->pointedToType), type->arrayData->size);
            }
            case tk_void: {
                return LLVMVoidType();
            }
            case tk_type: {
                return LLVMInt32Type();
            }
            case tk_invalid:
            case tk_const_string:
            case tk_const_number: {
                return LLVMVoidType();
            }
            default: {
                assert(false);
                return LLVMVoidType();
            }
        }
    }
}

LLVMValueRef protoTypeFunctionLLVM(functionId fid, LLVMModuleRef module) {
    Function* function = getFunctionFromId(fid);

    LLVMTypeRef returnType = typeIdToLLVM[function->returnType];
    LLVMTypeRef paramTypes[512];
    u64 paramCount = 0;
    for (u64 j = 0; j < function->numParams; j++) {
        paramTypes[j] = typeIdToLLVM[function->paramTypes[j]];
        paramCount++;
    }
    LLVMTypeRef functionType = LLVMFunctionType(returnType, paramTypes, paramCount, 0);
    LLVMValueRef functionValue = LLVMAddFunction(module, function->name, functionType);
    return functionValue;
}

void initCompiler() {
    typeIdToLLVM = arenaAlloc(gArena, sizeof(LLVMTypeRef) * typePool.size);
    for (u64 i = 0; i < typePool.size; i++) {
        typeIdToLLVM[i] = getLLVMType(i);
    }
    functionIdtoLLVMFunction = arenaAlloc(gArena, sizeof(LLVMValueRef) * functionPool.size);
    projectIdToLLVMModule = arenaAlloc(gArena, sizeof(LLVMModuleRef) * projectPool.size);
}

void compile(projectId projectId) {
    initCompiler();
    Project* project = getProjectFromId(projectId);
    LLVMModuleRef module = LLVMModuleCreateWithName(project->directory);
    projectIdToLLVMModule[projectId] = module;
    LLVMModuleRef mod = projectIdToLLVMModule[projectId];
    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        for (u64 j = 0; j < sourceCode->functionCount; j++) {
            functionId fid = sourceCode->functions[j];
            functionIdtoLLVMFunction[fid] = protoTypeFunctionLLVM(fid, mod);
        }
        for (u64 j = 0; j < sourceCode->functionCount; j++) {
            functionId fid = sourceCode->functions[j];
        }
    }
}
