#include "compiler.h"
#include "parser/arena.h"
#include "parser/type.h"
#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"
#include <stdio.h>
#include <string.h>

LLVMTypeRef* typeIdToLLVM;
LLVMValueRef* functionIdtoLLVMFunction;
LLVMTypeRef* fuctionIdToLLVMType;
LLVMModuleRef* projectIdToLLVMModule;
LLVMTargetDataRef dataLayout;
LLVMBuilderRef builder;

// we can override the value of variable
typedef struct _VariableOverride {
    LLVMValueRef value;
    void* misc;
    typeId type;
} VariableOverride;

LLVMValueRef compileExpression(Scope* scope, Expression* expression, projectId projectId);

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

void protoTypeFunctionLLVM(functionId fid, LLVMModuleRef module) {
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
    fuctionIdToLLVMType[fid] = functionType;
    functionIdtoLLVMFunction[fid] = functionValue;
}


LLVMValueRef compileBinaryOp(Scope* scope, Expression* expression, projectId projectId) {
    LLVMValueRef left = compileExpression(scope, expression->biopData->left, projectId);
    LLVMTypeRef llvmLeftType = typeIdToLLVM[expression->biopData->left->tid];
    LLVMValueRef right = compileExpression(scope, expression->biopData->right, projectId);
    LLVMTypeRef llvmRightType = typeIdToLLVM[expression->biopData->right->tid];
    Type* leftType = getTypeFromId(expression->biopData->left->tid);
    Type* rightType = getTypeFromId(expression->biopData->right->tid);

    switch (expression->biopData->operator) {
        case tt_add: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildAdd(builder, left, right, "addtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildAdd(builder, left, right, "addtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFAdd(builder, left, right, "faddtmp");
            } else if (leftType->kind == tk_int && rightType->kind == tk_pointer) {
                LLVMTypeRef underlyingType = typeIdToLLVM[rightType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, right, &left, 1, "ptradd");
            } else if (leftType->kind == tk_pointer && rightType->kind == tk_int) {
                LLVMTypeRef underlyingType = typeIdToLLVM[leftType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, left, &right, 1, "ptradd");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_pointer) {
                LLVMTypeRef underlyingType = typeIdToLLVM[rightType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, right, &left, 1, "ptradd");
            } else if (leftType->kind == tk_pointer && rightType->kind == tk_uint) {
                LLVMTypeRef underlyingType = typeIdToLLVM[leftType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, left, &right, 1, "ptradd");
            }
            assert(false);
            return NULL;
        }
        case tt_sub: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildSub(builder, left, right, "subtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildSub(builder, left, right, "subtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFSub(builder, left, right, "subtmp");
            } else if (leftType->kind == tk_int && rightType->kind == tk_pointer) {
                LLVMValueRef negated = LLVMBuildNeg(builder, left, "negtmp");
                LLVMTypeRef underlyingType = typeIdToLLVM[rightType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, right, &negated, 1, "ptrsub");
            } else if (leftType->kind == tk_pointer && rightType->kind == tk_int) {
                LLVMValueRef negated = LLVMBuildNeg(builder, right, "negtmp");
                LLVMTypeRef underlyingType = typeIdToLLVM[leftType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, left, &negated, 1, "ptrsub");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_pointer) {
                LLVMValueRef negated = LLVMBuildNeg(builder, left, "negtmp");
                LLVMTypeRef underlyingType = typeIdToLLVM[rightType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, right, &negated, 1, "ptrsub");
            } else if (leftType->kind == tk_pointer && rightType->kind == tk_uint) {
                LLVMValueRef negated = LLVMBuildNeg(builder, right, "negtmp");
                LLVMTypeRef underlyingType = typeIdToLLVM[leftType->pointedToType];
                return LLVMBuildGEP2(builder, underlyingType, left, &negated, 1, "ptrsub");
            } else if (leftType->kind == tk_pointer && rightType->kind == tk_pointer) {
                LLVMValueRef ptr1_int = LLVMBuildPtrToInt(builder, left, LLVMInt64Type(), "ptr1int");
                LLVMValueRef ptr2_int = LLVMBuildPtrToInt(builder, right, LLVMInt64Type(), "ptr2int");
                LLVMValueRef diff_bytes = LLVMBuildSub(builder, ptr1_int, ptr2_int, "ptrdiff");
                LLVMTypeRef underlyingType = typeIdToLLVM[leftType->pointedToType];
                uint64_t elem_size = LLVMABISizeOfType(dataLayout, underlyingType);
                LLVMValueRef elem_size_val = LLVMConstInt(LLVMInt64Type(), elem_size, 0);
                LLVMValueRef diff_elems = LLVMBuildSDiv(builder, diff_bytes, elem_size_val, "ptrdiff_elems");
                return diff_elems;
            }
            assert(false);
            return NULL;
        }
        case tt_mul: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildMul(builder, left, right, "multmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildMul(builder, left, right, "multmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFMul(builder, left, right, "multmp");
            }
            assert(false);
            return NULL;
        }
        case tt_div: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildSDiv(builder, left, right, "divtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildUDiv(builder, left, right, "divtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFDiv(builder, left, right, "divtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_mod: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildSRem(builder, left, right, "modtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildURem(builder, left, right, "modtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFRem(builder, left, right, "modtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_bit_and: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildAnd(builder, left, right, "andtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildAnd(builder, left, right, "andtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_bit_or: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildOr(builder, left, right, "ortmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildOr(builder, left, right, "ortmp");
            }
            assert(false);
            return NULL;
        }
        case tt_xor: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildXor(builder, left, right, "xortmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildXor(builder, left, right, "xortmp");
            }
            assert(false);
            return NULL;
        }
        case tt_and: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildAnd(builder, left, right, "andtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildAnd(builder, left, right, "andtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_or: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildOr(builder, left, right, "ortmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildOr(builder, left, right, "ortmp");
            }
            assert(false);
            return NULL;
        }
        case tt_eq: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "eqtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "eqtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealOEQ, left, right, "eqtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_neq: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntNE, left, right, "neqtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntNE, left, right, "neqtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealONE, left, right, "neqtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_lt: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntSLT, left, right, "lttmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntULT, left, right, "lttmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealOLT, left, right, "lttmp");
            }
            assert(false);
            return NULL;
        }
        case tt_gt: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "gttmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntUGT, left, right, "gttmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealOGT, left, right, "gttmp");
            }
            assert(false);
            return NULL;
        }
        case tt_leq: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntSLE, left, right, "leqtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntULE, left, right, "leqtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealOLE, left, right, "leqtmp");
            }
            assert(false);
            return NULL;
        }
        case tt_geq: {
            if (leftType->kind == tk_int && rightType->kind == tk_int) {
                return LLVMBuildICmp(builder, LLVMIntSGE, left, right, "geqtmp");
            } else if (leftType->kind == tk_uint && rightType->kind == tk_uint) {
                return LLVMBuildICmp(builder, LLVMIntUGE, left, right, "geqtmp");
            } else if (leftType->kind == tk_float && rightType->kind == tk_float) {
                return LLVMBuildFCmp(builder, LLVMRealOGE, left, right, "geqtmp");
            }
            assert(false);
            return NULL;
        }
        default: {
            assert(false);
            return NULL;
        }
    }
}

LLVMValueRef getConstantStringPtr(char* str, projectId projectId) {
    u64 len = strlen(str);
    LLVMModuleRef module = projectIdToLLVMModule[projectId];
    LLVMTypeRef strType = LLVMArrayType(LLVMInt8Type(), len + 1);
    char name[1024];
    static u64 i = 0;
    sprintf(name, "str%llu", i++);
    LLVMValueRef strConst = LLVMAddGlobal(module, strType, name);
    LLVMSetInitializer(strConst, LLVMConstString(str, len, /*DontNullTerminate=*/0));
    LLVMSetGlobalConstant(strConst, 1);
    LLVMSetLinkage(strConst, LLVMPrivateLinkage);
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
    LLVMValueRef indices[] = { zero, zero };
    return LLVMBuildGEP2(builder, strType, strConst, indices, 2, "strptr");
}

LLVMValueRef getConstantNumberValue(Expression* expr, typeId tid, Scope* scope, projectId projectId) {
    if (expr->type == ek_number) {
        return LLVMConstIntOfString(typeIdToLLVM[tid], expr->number, 10);
    }
    assert(false);
    return NULL;
}

LLVMValueRef compileImplicitCast(Scope* scope, Expression* expression, projectId projectId) {
    assert(expression->type == ek_implicit_cast);
    Type* castType = getTypeFromId(expression->tid);
    Type* castFromType = getTypeFromId(expression->implicitCast->tid);
    LLVMTypeRef llvmCastType = typeIdToLLVM[expression->tid];
    LLVMTypeRef llvmCastFromType = typeIdToLLVM[expression->implicitCast->tid];

    if (expression->implicitCast->type == ek_string) {
        bool isCharPtr = castType->kind == tk_pointer;
        bool formatString = expression->implicitCast->stringData->numStrings != 1;
        if (isCharPtr) {
            Type* underlyingType = getTypeFromId(castType->pointedToType);
            u64 intSize = underlyingType->numberSize;
            isCharPtr = intSize == 8 && underlyingType->kind == tk_uint;
        }
        if (isCharPtr) {
            if (formatString) {
                assert(false && "Not implemented");
            } else {
                return getConstantStringPtr(expression->implicitCast->stringData->strings[0], projectId);
            }
        }
        assert(false);
        return NULL;
    }

    LLVMValueRef value = compileExpression(scope, expression->implicitCast, projectId);

    if (castType->kind == tk_int && castFromType->kind == tk_int) {
        u64 castSize = castType->numberSize;
        u64 fromSize = castFromType->numberSize;
        if (castSize < fromSize) {
            LLVMValueRef intValue = LLVMBuildTrunc(builder, value, LLVMIntType(castSize), "intdowncast");
            return intValue;
        } else if (castSize > fromSize) {
            LLVMValueRef intValue = LLVMBuildSExt(builder, value, LLVMIntType(castSize), "intupcast");
            return intValue;
        }
        return value;
    }
    if (castType->kind == tk_uint && castFromType->kind == tk_uint) {
        u64 castSize = castType->numberSize;
        u64 fromSize = castFromType->numberSize;
        if (castSize < fromSize) {
            LLVMValueRef intValue = LLVMBuildTrunc(builder, value, LLVMIntType(castSize), "uintdowncast");
            return intValue;
        } else if (castSize > fromSize) {
            LLVMValueRef intValue = LLVMBuildZExt(builder, value, LLVMIntType(castSize), "uintupcast");
            return intValue;
        }
        return value;
    }
    if (castType->kind == tk_float && castFromType->kind == tk_float) {
        u64 castSize = castType->numberSize;
        u64 fromSize = castFromType->numberSize;
        if (castSize < fromSize) {
            LLVMValueRef intValue = LLVMBuildFPTrunc(builder, value, llvmCastType, "floatdowncast");
            return intValue;
        } else if (castSize > fromSize) {
            LLVMValueRef intValue = LLVMBuildFPExt(builder, value, llvmCastType, "floatupcast");
            return intValue;
        }
        return value;
    }

    if (castType->kind == tk_int && castFromType->kind == tk_uint) {
        u64 castSize = castType->numberSize;
        u64 fromSize = castFromType->numberSize;
        if (castSize < fromSize) {
            LLVMValueRef intValue = LLVMBuildTrunc(builder, value, LLVMIntType(castSize), "uinttoint");
            return intValue;
        } else if (castSize > fromSize) {
            LLVMValueRef intValue = LLVMBuildZExt(builder, value, LLVMIntType(castSize), "uinttoint");
            return intValue;
        }
        return value;
    }
    if (castType->kind == tk_uint && castFromType->kind == tk_int) {
        u64 castSize = castType->numberSize;
        u64 fromSize = castFromType->numberSize;
        if (castSize < fromSize) {
            LLVMValueRef intValue = LLVMBuildTrunc(builder, value, LLVMIntType(castSize), "inttouint");
            return intValue;
        } else if (castSize > fromSize) {
            LLVMValueRef intValue = LLVMBuildSExt(builder, value, LLVMIntType(castSize), "inttouint");
            return intValue;
        }
        return value;
    }

    if (castType->kind == tk_int && castFromType->kind == tk_float) {
        LLVMValueRef intValue = LLVMBuildFPToSI(builder, value, llvmCastType, "floattoint");
        return intValue;
    }
    if (castType->kind == tk_float && castFromType->kind == tk_int) {
        LLVMValueRef intValue = LLVMBuildSIToFP(builder, value, llvmCastType, "inttofloat");
        return intValue;
    }

    if (castType->kind == tk_uint && castFromType->kind == tk_float) {
        LLVMValueRef intValue = LLVMBuildFPToUI(builder, value, llvmCastType, "floattouint");
        return intValue;
    }
    if (castType->kind == tk_float && castFromType->kind == tk_uint) {
        LLVMValueRef intValue = LLVMBuildUIToFP(builder, value, llvmCastType, "uinttofloat");
        return intValue;
    }

    if (castType->kind == tk_int && castFromType->kind == tk_const_number) {
        return getConstantNumberValue(expression->implicitCast, expression->tid, scope, projectId);
    }
    if (castType->kind == tk_uint && castFromType->kind == tk_const_number) {
        return getConstantNumberValue(expression->implicitCast, expression->tid, scope, projectId);
    }
    if (castType->kind == tk_float && castFromType->kind == tk_const_number) {
        return getConstantNumberValue(expression->implicitCast, expression->tid, scope, projectId);
    }

    if (castType->kind == tk_pointer && castFromType->kind == tk_pointer) {
        return value;
    }

    if (castType->kind == tk_pointer && castFromType->kind == tk_int) {
        LLVMValueRef intAsPtr = LLVMBuildIntToPtr(builder, value, llvmCastType, "inttoptr");
        return intAsPtr;
    }
    if (castType->kind == tk_int && castFromType->kind == tk_pointer) {
        LLVMValueRef ptrAsInt = LLVMBuildPtrToInt(builder, value, llvmCastType, "ptrtoint");
        return ptrAsInt;
    }

    if (castType->kind == tk_pointer && castFromType->kind == tk_uint) {
        LLVMValueRef intAsPtr = LLVMBuildIntToPtr(builder, value, llvmCastType, "uinttoptr");
        return intAsPtr;
    }
    if (castType->kind == tk_uint && castFromType->kind == tk_pointer) {
        LLVMValueRef ptrAsInt = LLVMBuildPtrToInt(builder, value, llvmCastType, "ptrtoint");
        return ptrAsInt;
    }



    assert(false);
    return NULL;
}

LLVMValueRef compileFunctionCall(Scope* scope, Expression* expression, projectId projectId) {
    functionId fid = expression->functionCallData->functionId;
    Function* function = getFunctionFromId(fid);
    FunctionCallData* functionCallData = expression->functionCallData;

    LLVMValueRef* paramValues = arenaAlloc(gArena, sizeof(LLVMValueRef) * functionCallData->numParameters);
    for (u64 i = 0; i < functionCallData->numParameters; i++) {
        Expression* parameter = &functionCallData->parameters[i];
        LLVMValueRef paramValue = compileExpression(scope, parameter, projectId);
        paramValues[i] = paramValue;
    }

    LLVMValueRef functionValue = functionIdtoLLVMFunction[fid];
    LLVMTypeRef functionType = fuctionIdToLLVMType[fid];
    return LLVMBuildCall2(builder, functionType, functionValue, paramValues, functionCallData->numParameters, "calltmp");
}

LLVMValueRef compileDeref(Scope* scope, Expression* expression, projectId projectId) {
    assert(expression->type == ek_deref);

    LLVMValueRef value = compileExpression(scope, expression->deref, projectId);
    LLVMTypeRef underlyingType = typeIdToLLVM[expression->deref->tid];
    Type* type = getTypeFromId(expression->deref->tid);
    if (type->kind == tk_ref) {
        return LLVMBuildLoad2(builder, underlyingType, value, "deref");
    } else if (type->kind == tk_pointer) {
        return value;
    }
    assert(false);
    return NULL;
}

LLVMValueRef compileVariable(Scope* scope, Expression* expression, projectId projectId) {
    assert(expression->type == ek_variable);
    VariableOverride* variable = (VariableOverride*)getVariableByName(scope, expression->variable);
    assert(variable != NULL);
    return variable->value;
}

LLVMValueRef compileExpression(Scope* scope, Expression* expression, projectId projectId) {
    LLVMTypeRef llvmType = typeIdToLLVM[expression->tid];
    switch (expression->type) {
        case ek_number: {
            return NULL;
        }
        case ek_biop: {
            return compileBinaryOp(scope, expression, projectId);
        }
        case ek_implicit_cast: {
            return compileImplicitCast(scope, expression, projectId);
        }
        case ek_function_call: {
            return compileFunctionCall(scope, expression, projectId);
        }
        case ek_deref: {
            return compileDeref(scope, expression, projectId);
        }
        case ek_variable: {
            return compileVariable(scope, expression, projectId);
        }
        default: {
            assert(false);
            return NULL;
        }
    }
}

void compileExpressionStatement(Scope* scope, Statement* statement, u64* childIndex, projectId projectId) {
    assert(statement->kind == sk_expression);
    LLVMValueRef value = compileExpression(scope, statement->expressionData, projectId);
}

void compileReturnStatement(Scope* scope, Statement* statement, u64* childIndex, projectId projectId) {
    assert(statement->kind == sk_return);
    LLVMValueRef value = compileExpression(scope, statement->expressionData, projectId);
    LLVMBuildRet(builder, value);
}

void compileScope(Scope* scope, projectId projectId) {
    u64 childIndex = 0;
    bool stopBuildingScope = false;
    for (u64 i = 0; i < scope->statementsCount; i++) {
        Statement* statement = &scope->statements[i];
        switch (statement->kind) {
            case sk_expression: {
                compileExpressionStatement(scope, statement, &childIndex, projectId);
                break;
            }
            case sk_return: {
                stopBuildingScope = true;
                compileReturnStatement(scope, statement, &childIndex, projectId);
                break;
            }
            default: {
                assert(false);
            }
        }
    }
}


void compileFunction(functionId fid, projectId projectId) {
    Function* function = getFunctionFromId(fid);
    if (function->isExtern || function->isExternC > 0) {
        return;
    }
    LLVMValueRef functionValue = functionIdtoLLVMFunction[fid];
    LLVMTypeRef functionType = LLVMTypeOf(functionValue);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(functionValue, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // allocate variables onto the stack
    Scope* s = &function->scope;
    while (s != NULL) {
        for (u64 i = 0; i < s->varilablesCount; i++) {
            // we can override the value of variable
            VariableOverride* variable = (VariableOverride*)&s->variables[i];
            LLVMTypeRef llvmType = typeIdToLLVM[variable->type];
            static u64 i = 0;
            char name[1024];
            sprintf(name, "var%llu", i++);
            LLVMValueRef allocaInst = LLVMBuildAlloca(builder, llvmType, name);
            LLVMBuildStore(builder, LLVMConstNull(llvmType), allocaInst);
            variable->value = allocaInst;
        }
        s = s->parent;
    }

    compileScope(&function->scope, projectId);
}


void compileProject(projectId projectId) {
    Project* project = getProjectFromId(projectId);
    LLVMModuleRef module = LLVMModuleCreateWithName(project->directory);
    projectIdToLLVMModule[projectId] = module;
    LLVMModuleRef mod = projectIdToLLVMModule[projectId];
    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        for (u64 j = 0; j < sourceCode->functionCount; j++) {
            functionId fid = sourceCode->functions[j];
            protoTypeFunctionLLVM(fid, mod);
        }
        for (u64 j = 0; j < sourceCode->functionCount; j++) {
            functionId fid = sourceCode->functions[j];
            compileFunction(fid, projectId);
        }
    }
}

void outputObjectFiles(projectId projectId) {
}

void compile(projectId projectId) {
    // Init
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    char* triple = LLVMGetDefaultTargetTriple();
    printf("triple: %s\n", triple);
    LLVMTargetRef target;
    char* error = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMTargetMachineRef targetMachine = LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
    dataLayout = LLVMCreateTargetDataLayout(targetMachine);
    typeIdToLLVM = arenaAlloc(gArena, sizeof(LLVMTypeRef) * typePool.size);
    for (u64 i = 0; i < typePool.size; i++) {
        typeIdToLLVM[i] = getLLVMType(i);
    }
    functionIdtoLLVMFunction = arenaAlloc(gArena, sizeof(LLVMValueRef) * functionPool.size);
    fuctionIdToLLVMType = arenaAlloc(gArena, sizeof(LLVMTypeRef) * functionPool.size);
    projectIdToLLVMModule = arenaAlloc(gArena, sizeof(LLVMModuleRef) * projectPool.size);
    builder = LLVMCreateBuilder();

    // ------------------------------------------------------------------------------------------------
    // compile the project
    compileProject(projectId);
    // ------------------------------------------------------------------------------------------------
    // output
    LLVMModuleRef llvmModule = projectIdToLLVMModule[projectId];

    if (LLVMVerifyModule(llvmModule, LLVMPrintMessageAction, &error) != 0) {
        fprintf(stderr, "LLVMVerifyModule failed: %s\n", error);
        LLVMDisposeMessage(error);
        assert(false);
    }

    char* outputFileName = "output.obj";
    if (LLVMTargetMachineEmitToFile(targetMachine, llvmModule, outputFileName, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        assert(false);
    }

    char objFileName[1024];
    sprintf(objFileName, "output_%llu.obj", (u64)projectId);
    if (LLVMTargetMachineEmitToFile(targetMachine, llvmModule, objFileName, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
    }


    LLVMDisposeTargetMachine(targetMachine);
    LLVMDisposeMessage(triple);
}
