#include "parser.h"
#include "parser/function.h"
#include <assert.h>
#include <string.h>

void addStatementToScope(Scope* scope, Statment* statement, Project* project) {
    if (scope->statementCount >= scope->statementCapacity) {
        scope->statementCapacity = scope->statementCapacity ? scope->statementCapacity * 2 : 16;
        Statment* newStatements = arenaAlloc(project->arena, sizeof(Statment) * scope->statementCapacity);
        if (!newStatements) {
            logError("Failed to allocate memory for statements in scope");
            return;
        }
        if (scope->statements) {
            memcpy(newStatements, scope->statements, sizeof(Statment) * scope->statementCount);
        }
        scope->statements = newStatements;
    }
    scope->statements[scope->statementCount++] = *statement;
}

Variable* getVariableFromTopScope(Scope* scope, const char* name) {
    for (u64 i = 0; i < scope->variableCount; i++) {
        if (strcmp(scope->variables[i].name, name) == 0) {
            return &scope->variables[i];
        }
    }
    return NULL;  // Variable not found in this scope
}

Variable* getVariableFromScope(Scope* scope, const char* name) {
    for (u64 i = 0; i < scope->variableCount; i++) {
        char* varName = scope->variables[i].name;
        if (strcmp(varName, name) == 0) {
            return &scope->variables[i];
        }
    }
    if (scope->parent) {
        return getVariableFromScope(scope->parent, name);
    }
    return NULL;
}

void addVariableToScope(Scope* scope, Variable* variable, Project* project) {
    if (scope->variableCount >= scope->variableCapacity) {
        scope->variableCapacity = scope->variableCapacity ? scope->variableCapacity * 2 : 16;
        Variable* newVariables = arenaAlloc(project->arena, sizeof(Variable) * scope->variableCapacity);
        if (!newVariables) {
            logError("Failed to allocate memory for variables in scope");
            return;
        }
        if (scope->variables) {
            memcpy(newVariables, scope->variables, sizeof(Variable) * scope->variableCount);
        }
        scope->variables = newVariables;
    }
    scope->variables[scope->variableCount++] = *variable;
}

void implementScope(Scope* scope, functionId funcId, Token* startToken, Project* project) {
    assert(startToken->type == tt_lbrace);

    startToken++;
    Token* token = startToken;
    while (true) {
        if (token->type == tt_rbrace) {
            token++;
            break;  // End of scope
        }
        if (token->type == tt_endl) {
            token++;  // Skip empty line
            continue;
        }
        Statment statement = createStatmentFromTokens(&token, funcId, scope, project);
        if (statement.type == st_error) {
            //skip line if error
            while (token->type != tt_endl) {
                token++;
            }
        }
        addStatementToScope(scope, &statement, project);

        // we messed up some implementation if it is not endl here
        assert(token->type == tt_endl);
    }
}
