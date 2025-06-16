#include "compiler.h"
#include "parser.h"

#include <assert.h>

CompilerType* gCompilerTypes = NULL;

void createCompilerTypes() {
    gCompilerTypes = arenaAlloc(gTypesArena, sizeof(CompilerType) * gTypesCount);
    for (u64 i = 1; i < gTypesCount; i++) {
        typeId tid = i;
        CompilerType compilerType = createCompilerType(tid);
    }
}

CompilerType createCompilerType(typeId tid) {
    CompilerType compilerType = { 0 };
    compilerType.tid = tid;
    Type* type = getTypeFromId(tid);
    switch (type->kind) {
        case tk_const_int:
        case tk_const_uint: {
            compilerType.llvmType = LLVMIntType(64);
            break;
        }
        case tk_const_float: {
            compilerType.llvmType = LLVMDoubleType();
            break;
        }
        case tk_const_string: {
            // char*
            compilerType.llvmType = LLVMPointerType(LLVMVoidType(), 0);
            break;
        }
        case tk_invalid: {
            assert(false && "Invalid type");
            break;
        }
        case tk_ptr: {
            compilerType.llvmType = LLVMPointerType(LLVMVoidType(), 0);
            break;
        }
        case tk_uint:
        case tk_int: {
            u64 numberSize = type->numberSize;
            compilerType.llvmType = LLVMIntType(numberSize);
            break;
        }
        case tk_float: {
            u64 numberSize = type->numberSize;
            if (numberSize == 4) {
                compilerType.llvmType = LLVMFloatType();
            } else if (numberSize == 8) {
                compilerType.llvmType = LLVMDoubleType();
            } else if (numberSize == 2) {
                compilerType.llvmType = LLVMHalfType();
            } else {
                assert(false && "Invalid number size for float type");
            }
            break;
        }
        case tk_void: {
            compilerType.llvmType = LLVMVoidType();
            break;
        }
        case tk_struct: {
            assert(false && "Struct type not implemented yet");
            break;
        }
        case tk_enum: {
            assert(false && "Enum type not implemented yet");
            break;
        }
        default: {
            assert(false && "Invalid type kind");
        }
    }
    gCompilerTypes[tid] = compilerType;
    return compilerType;
}
CompilerType getCompilerTypeFromId(typeId typeId) {
    return gCompilerTypes[typeId];
}
