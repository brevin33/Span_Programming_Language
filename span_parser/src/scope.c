#include "span_parser.h"

SpanScope createSpanScope(SpanAst* ast, SpanScope* parent, SpanFunction* function) {
    SpanScope scope;
    massert(ast->type == ast_scope, "should be a scope");
    SpanAstScope* astScope = &ast->scope;
    scope.ast = ast;
    scope.parent = parent;
    scope.variablesCount = 0;
    scope.variablesCapacity = 2;
    scope.variables = allocArena(context.arena, sizeof(SpanVariable) * scope.variablesCapacity);
    return scope;
}

void addStatmentsToScope(SpanAst* ast, SpanScope* scope, SpanFunction* function) {
    massert(ast->type == ast_scope, "should be a scope");
    SpanAstScope* astScope = &ast->scope;
    scope->statmentsCount = astScope->statementsCount;
    if (astScope->statementsCount > 0) scope->statments = allocArena(context.arena, sizeof(SpanStatement) * astScope->statementsCount);
    else
        scope->statments = NULL;
    for (u64 i = 0; i < astScope->statementsCount; i++) {
        SpanAst* ast = astScope->statements + i;
        scope->statments[i] = createSpanStatement(ast, scope, function);
    }
}

void compileScope(SpanScope* scope, SpanFunction* function) {
    LLVMBasicBlockRef scopeBlock = LLVMAppendBasicBlock(function->llvmFunc, "scope");
    LLVMBuildBr(context.builder, scopeBlock);
    LLVMPositionBuilderAtEnd(context.builder, scopeBlock);
    context.currentBlock = scopeBlock;

    // compile variables
    for (u64 i = 0; i < scope->variablesCount; i++) {
        SpanVariable* variable = &scope->variables[i];
        SpanType type = variable->type;
        LLVMTypeRef llvmType = getLLVMType(&type);
        variable->llvmValue = LLVMBuildAlloca(context.builder, llvmType, variable->name);
    }

    // compile statements
    bool exitedScope = false;
    for (u64 i = 0; i < scope->statmentsCount; i++) {
        SpanStatement* statement = &scope->statments[i];
        exitedScope = compileStatement(statement, scope, function);
        if (exitedScope) {
            break;
        }
    }

    if (!exitedScope) {
        LLVMBasicBlockRef nextBlock = LLVMAppendBasicBlock(function->llvmFunc, "next");
        LLVMBuildBr(context.builder, nextBlock);
        LLVMPositionBuilderAtEnd(context.builder, nextBlock);
    }
}

SpanVariable* addVariableToScope(SpanScope* scope, char* name, SpanType type, SpanAst* ast) {
    for (u64 i = 0; i < scope->variablesCount; i++) {
        SpanVariable* variable = &scope->variables[i];
        if (strcmp(variable->name, name) == 0) {
            logErrorAst(ast, "variable already exists in scope with same name");
            //TODO: figure out we shuold do anything than log error
        }
    }
    SpanVariable variable;
    u64 nameLength = strlen(name);
    variable.name = allocArena(context.arena, nameLength + 1);
    memcpy(variable.name, name, nameLength + 1);
    variable.type = type;
    variable.ast = ast;
    if (scope->variablesCount >= scope->variablesCapacity) {
        scope->variables = reallocArena(context.arena, sizeof(SpanVariable) * scope->variablesCapacity * 2, scope->variables, sizeof(SpanVariable) * scope->variablesCapacity);
        scope->variablesCapacity *= 2;
    }
    scope->variables[scope->variablesCount++] = variable;
    return &scope->variables[scope->variablesCount - 1];
}

SpanVariable* getVariableFromScope(SpanScope* scope, char* name) {
    for (u64 i = 0; i < scope->variablesCount; i++) {
        SpanVariable* variable = &scope->variables[i];
        if (strcmp(variable->name, name) == 0) {
            return variable;
        }
    }
    if (scope->parent == NULL) {
        return NULL;
    }
    return getVariableFromScope(scope->parent, name);
}
