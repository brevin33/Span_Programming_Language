#include "parser/statment.h"
#include "parser.h"
#include "parser/expression.h"
#include "parser/function.h"
#include "parser/scope.h"
#include "parser/tokens.h"
#include <assert.h>
#include <string.h>

Declaration createDeclarationFromTokens(Token** tokens, functionId function, Scope* scope, Project* project) {
    Declaration declaration = { 0 };
    Token* token = *tokens;

    typeId type = getTypeIdFromToken(&token);
    if (type == 0) {
        logErrorToken("Expected type for declaration", project, token);
        return declaration;
    }
    declaration.variable.type = type;

    if (token->type != tt_id) {
        logErrorToken("Expected identifier for declaration", project, token);
        return declaration;
    }
    char* varName = arenaAlloc(project->arena, strlen(token->str) + 1);
    memcpy(varName, token->str, strlen(token->str) + 1);
    token++;
    declaration.variable.name = varName;

    //add variable to scope
    Variable* existingVar = getVariableFromTopScope(scope, varName);
    if (existingVar != NULL) {
        logErrorToken("Variable already declared in this scope", project, token);
        declaration.variable.name = NULL;  // Clear variable name on error
        declaration.variable.type = 0;  // Clear type on error
        return declaration;
    }

    addVariableToScope(scope, &declaration.variable, project);

    if (token->type != tt_assign && token->type != tt_add_assign && token->type != tt_sub_assign && token->type != tt_mul_assign && token->type != tt_div_assign && token->type != tt_mod_assign) {
        // If no assignment operator, return declaration without assignment
        declaration.assignment = NULL;
        *tokens = token;
        return declaration;
    }

    Token* assignToken = token - 1;
    Assignment a = createAssignmentFromTokens(&assignToken, function, scope, project);
    if (a.variableName == NULL) {
        // If assignment failed, clear variable name and type
        declaration.variable.name = NULL;  // Clear variable name on error
        declaration.variable.type = 0;  // Clear type on error
        return declaration;
    }
    token = assignToken;
    declaration.assignment = arenaAlloc(project->arena, sizeof(Assignment));
    memcpy(declaration.assignment, &a, sizeof(Assignment));
    *tokens = token;
    return declaration;
}


OurTokenType assignOpToBiop(OurTokenType assignOp) {
    switch (assignOp) {
        case tt_assign:
            return tt_assign;
        case tt_add_assign:
            return tt_add;
        case tt_sub_assign:
            return tt_sub;
        case tt_mul_assign:
            return tt_mul;
        case tt_div_assign:
            return tt_div;
        case tt_mod_assign:
            return tt_mod;
        default:
            return tt_error;  // Invalid assignment operator
    }
}

Assignment createAssignmentFromTokens(Token** tokens, functionId funcId, Scope* scope, Project* project) {
    Function* function = getFunctionFromId(funcId);
    Token* token = *tokens;
    Assignment assignment = { 0 };
    assert(token->type != tt_eof || token->type != tt_endl);

    if (token->type != tt_id) {
        logErrorToken("Expected identifier for assignment", project, token);
        assignment.variableName = NULL;  // Clear variable name on error
        return assignment;
    }

    Variable* var = getVariableFromScope(scope, token->str);
    Expresstion varExp = { 0 };
    varExp.type = et_variable;
    varExp.tid = var ? var->type : 0;
    varExp.variable = arenaAlloc(project->arena, strlen(token->str) + 1);
    memcpy(varExp.variable, token->str, strlen(token->str) + 1);
    if (var == NULL) {
        logErrorToken("Variable not declared before assignment", project, token);
        return assignment;
    }

    assignment.variableName = arenaAlloc(project->arena, strlen(token->str) + 1);
    memcpy(assignment.variableName, token->str, strlen(token->str) + 1);
    token++;

    if (token->type != tt_assign && token->type != tt_add_assign && token->type != tt_sub_assign && token->type != tt_mul_assign && token->type != tt_div_assign && token->type != tt_mod_assign) {
        logErrorToken("Expected assignment operator", project, token);
        assignment.variableName = NULL;  // Clear variable name on error
        return assignment;
    }

    OurTokenType assignOp = token->type;
    token++;

    Expresstion value = createExpresstionFromTokens(&token, tt_endl, funcId, scope, project);

    if (value.type == et_error) {
        assignment.variableName = NULL;  // Clear variable name on error
        return assignment;
    }

    switch (assignOp) {
        case tt_assign:
            break;
        case tt_mul_assign:
        case tt_div_assign:
        case tt_mod_assign:
        case tt_sub_assign:
        case tt_add_assign: {
            Expresstion biop;
            BiopExpresstion biopExp;
            biopExp.left = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(biopExp.left, &varExp, sizeof(Expresstion));
            biopExp.operator= assignOpToBiop(assignOp);
            biopExp.right = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(biopExp.right, &value, sizeof(Expresstion));
            biop.type = et_biop;
            biop.tid = getBiopTypeResult(&biopExp, project);
            biop.biopExpresstion = arenaAlloc(project->arena, sizeof(BiopExpresstion));
            memcpy(biop.biopExpresstion, &biopExp, sizeof(BiopExpresstion));
            value = biop;
            break;
        }
        default:
            logErrorToken("Invalid assignment operator", project, token);
            assignment.variableName = NULL;  // Clear variable name on error
            return assignment;
    }

    bool staticCast = canImplCast(value.tid, var->type, project);
    if (!staticCast) {
        logErrorToken("Type mismatch in assignment", project, token);
        assignment.variableName = NULL;  // Clear variable name on error
        return assignment;
    }

    assignment.value = arenaAlloc(project->arena, sizeof(Expresstion));
    memcpy(assignment.value, &value, sizeof(Expresstion));

    *tokens = token;
    return assignment;
}

Statment createStatmentFromTokens(Token** tokens, functionId funcId, Scope* scope, Project* project) {
    Function* function = getFunctionFromId(funcId);
    Token* token = *tokens;
    Statment statement = { .type = st_error };
    assert(token->type != tt_eof && token->type != tt_endl);


    switch (token->type) {
        case tt_id: {
            // figure out if assignment declaration or expresstion
            Token* startToken = token;
            typeId type = getTypeIdFromToken(&token);
            if (type != 0) {
                if (token->type == tt_dot) {
                    // expressiont because it starts with type getting type infomation
                    token = startToken;
                    Expresstion expresstion = createExpresstionFromTokens(tokens, tt_endl, funcId, scope, project);
                    if (expresstion.type == et_error) {
                        return statement;
                    }
                    statement.type = st_expression;
                    statement.expression = arenaAlloc(project->arena, sizeof(Expresstion));
                    memcpy(statement.expression, &expresstion, sizeof(Expresstion));
                    *tokens = token;
                    return statement;
                }
                // must be a declaration
                token = startToken;
                Declaration declaration = createDeclarationFromTokens(&token, funcId, scope, project);
                if (declaration.variable.name == NULL) {
                    return statement;  // Error in declaration
                }
                statement.type = st_declaration;
                statement.declaration = arenaAlloc(project->arena, sizeof(Declaration));
                memcpy(statement.declaration, &declaration, sizeof(Declaration));
                *tokens = token;
                return statement;
            }
            // check line for assignment token
            bool isAssignment = false;
            while (token->type != tt_endl && token->type != tt_eof) {
                if (token->type == tt_assign || token->type == tt_add_assign || token->type == tt_sub_assign || token->type == tt_mul_assign || token->type == tt_div_assign || token->type == tt_mod_assign) {
                    isAssignment = true;
                    break;
                }
                token++;
            }
            if (isAssignment) {
                token = startToken;
                Assignment assignment = createAssignmentFromTokens(&token, funcId, scope, project);
                if (assignment.variableName == NULL) {
                    return statement;
                }
                statement.type = st_assignment;
                statement.assignment = arenaAlloc(project->arena, sizeof(Assignment));
                memcpy(statement.assignment, &assignment, sizeof(Assignment));
                *tokens = token;
                return statement;
            } else {
                token = startToken;
                Expresstion expresstion = createExpresstionFromTokens(tokens, tt_endl, funcId, scope, project);
                if (expresstion.type == et_error) {
                    return statement;
                }
                statement.type = st_expression;
                statement.expression = arenaAlloc(project->arena, sizeof(Expresstion));
                memcpy(statement.expression, &expresstion, sizeof(Expresstion));
                *tokens = token;
                return statement;
            }
        }
        case tt_return: {
            token++;
            Expresstion returnValue = createExpresstionFromTokens(&token, tt_endl, funcId, scope, project);
            if (returnValue.type == et_error) {
                return statement;
            }
            statement.type = st_return;
            statement.returnValue = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(statement.returnValue, &returnValue, sizeof(Expresstion));
            *tokens = token;
            return statement;
        }
        case tt_if: {
            token++;
            OurTokenType dels[] = { tt_endl, tt_lbrace };
            Expresstion ifCondition = createExpresstionFromTokensDels(&token, dels, 2, funcId, scope, project);
            if (ifCondition.type == et_error) {
                return statement;
            }
            if (token->type == tt_lbrace) {
                token++;
                if (token->type != tt_endl) {
                    logErrorToken("Can't have any more statments on line after if", project, token);
                    statement.type = st_error;
                    return statement;
                }
            }
            statement.type = st_if;
            statement.ifCondition = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(statement.ifCondition, &ifCondition, sizeof(Expresstion));
            *tokens = token;
            return statement;
        }
        case tt_break: {
            token++;
            u64 breakLevel = 1;
            if (token->type == tt_int) {
                breakLevel = getTokenInt(token);
                token++;
            }
            if (token->type != tt_endl) {
                logErrorToken("Can't have any more statments on line after break", project, token);
                statement.type = st_error;
                return statement;
            }
            statement.type = st_break;
            statement.breakLevel = breakLevel;
            *tokens = token;
            return statement;
        }
        case tt_continue: {
            token++;
            u64 continueLevel = 1;
            if (token->type == tt_int) {
                continueLevel = getTokenInt(token);
                token++;
            }
            if (token->type != tt_endl) {
                logErrorToken("Can't have any more statments on line after continue", project, token);
                statement.type = st_error;
                return statement;
            }
            statement.type = st_continue;
            statement.continueLevel = continueLevel;
            *tokens = token;
            return statement;
        }
        default: {
            logErrorToken("Unexpected token in statement", project, token);
            while (token->type != tt_endl && token->type != tt_eof) {
                token++;
            }
            return statement;
        }
    }
}
