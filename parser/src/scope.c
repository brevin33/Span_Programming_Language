#include "parser.h"
#include "parser/map.h"
#include <string.h>

Scope createScope(functionId function, Arena* arena) {
    Scope scope = { 0 };
    scope.arena = arena;
    scope.function = function;
    scope.varilablesCount = 0;
    scope.varilablesCapacity = 1;
    scope.variables = arenaAlloc(arena, sizeof(Variable) * scope.varilablesCapacity);
    scope.nameToVariable = createMapArena(arena);
    scope.childrenCount = 0;
    scope.childrenCapacity = 1;
    scope.children = arenaAlloc(arena, sizeof(Scope) * scope.childrenCapacity);
    scope.statementsCount = 0;
    scope.statementsCapacity = 1;
    scope.statements = arenaAlloc(arena, sizeof(Statement) * scope.statementsCapacity);
    return scope;
}
void addVariableToScope(Scope* scope, Token* token, char* name, typeId type) {
    assert(scope->variables != NULL);
    if (scope->varilablesCount >= scope->varilablesCapacity) {
        scope->variables = arenaRealloc(scope->arena, scope->variables, sizeof(Variable) * scope->varilablesCapacity, sizeof(Variable) * scope->varilablesCapacity * 2);
        scope->varilablesCapacity *= 2;
    }
    Variable* variable = &scope->variables[scope->varilablesCount];
    variable->token = token;
    u64 nameSize = strlen(name) + 1;
    variable->name = arenaAlloc(scope->arena, nameSize);
    memcpy(variable->name, name, nameSize);
    variable->type = type;

    mapSet(&scope->nameToVariable, variable->name, variable);
}

void addStatementToScope(Scope* scope, Statement* statement) {
    assert(scope->statements != NULL);
    if (scope->statementsCount >= scope->statementsCapacity) {
        scope->statements = arenaRealloc(scope->arena, scope->statements, sizeof(Statement) * scope->statementsCapacity, sizeof(Statement) * scope->statementsCapacity * 2);
        scope->statementsCapacity *= 2;
    }
    scope->statements[scope->statementsCount] = *statement;
    scope->statementsCount++;
}


void addChildToScope(Scope* scope, Scope* child) {
    assert(scope->children != NULL);
    child->parent = scope;
    if (scope->childrenCount >= scope->childrenCapacity) {
        scope->children = arenaRealloc(scope->arena, scope->children, sizeof(Scope) * scope->childrenCapacity, sizeof(Scope) * scope->childrenCapacity * 2);
        scope->childrenCapacity *= 2;
    }
    scope->children[scope->childrenCount] = *child;
    scope->childrenCount++;
}

Variable* getVariableByName(Scope* scope, char* name) {
    Variable** variable = (Variable**)mapGet(&scope->nameToVariable, name);
    if (variable == NULL) {
        if (scope->parent != NULL) {
            return getVariableByName(scope->parent, name);
        } else {
            return NULL;
        }
    }
    return *variable;
}


void implementScope(Scope* scope, Token** tokens) {
    Token* token = *tokens;
    assert(token->type == tt_lbrace);
    token++;
    while (token->type != tt_rbrace) {
        if (token->type == tt_endl) {
            token++;
            continue;
        }
        Statement statement = createStatmentFromTokens(&token, scope->function, scope);
        if (statement.kind == sk_invalid) {
            int braceCount = 0;
            while (token->type != tt_endl || braceCount != 0) {
                if (token->type == tt_lbrace) {
                    braceCount++;
                }
                if (token->type == tt_rbrace) {
                    braceCount--;
                }
                assert(braceCount >= 0);
                token++;
            }
            continue;
        }
        if (scope->statementsCount >= scope->statementsCapacity) {
            scope->statements = arenaRealloc(scope->arena, scope->statements, sizeof(Statement) * scope->statementsCapacity, sizeof(Statement) * scope->statementsCapacity * 2);
            scope->statementsCapacity *= 2;
        }
        scope->statements[scope->statementsCount] = statement;
        scope->statementsCount++;
        assert(token->type == tt_endl || token->type == tt_rbrace);
    }
    token++;
    *tokens = token;
}

bool variableExistsInScope(Scope* scope, char* name) {
    Variable** variable = (Variable**)mapGet(&scope->nameToVariable, name);
    if (variable == NULL) {
        return false;
    }
    return true;
}
