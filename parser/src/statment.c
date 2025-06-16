#include "parser/statment.h"
#include "parser.h"
#include "parser/expression.h"
#include "parser/function.h"
#include "parser/scope.h"
#include "parser/tokens.h"
#include <assert.h>
#include <string.h>

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

AssignmentLeftSideElement createAssignmentLeftSideElementFromTokens(Token** tokens, functionId funcId, Scope* scope, Project* project) {
    Token* token = *tokens;
    AssignmentLeftSideElement ase = { 0 };
    typeId type = getTypeIdFromToken(&token);
    if (type == 0) {
        // might be a variable
        if (token->type != tt_id) {
            logErrorToken("Expected identifier for assignment", project, token);
            ase.type = ase_error;
            return ase;
        }
        Variable* var = getVariableFromScope(scope, token->str);
        if (var == NULL) {
            logErrorToken("Variable not declared before assignment", project, token);
            ase.type = ase_error;
            return ase;
        }
        ase.type = ase_variable;
        char* varName = arenaAlloc(project->arena, strlen(token->str) + 1);
        memcpy(varName, token->str, strlen(token->str) + 1);
        ase.variable.name = varName;
        ase.variable.type = var->type;
        token++;
        return ase;
    }
    // must be a declaration
    if (token->type != tt_id) {
        logErrorToken("Expected name for variable declaration", project, token);
        ase.type = ase_error;
        return ase;
    }
    char* varName = arenaAlloc(project->arena, strlen(token->str) + 1);
    memcpy(varName, token->str, strlen(token->str) + 1);
    token++;
    ase.type = ase_declaration;
    ase.variable.name = varName;
    ase.variable.type = type;
    addVariableToScope(scope, &ase.variable, project);
    *tokens = token;
    return ase;
}

Assignment createAssignmentFromTokens(Token** tokens, functionId funcId, Scope* scope, Project* project) {
    Function* function = getFunctionFromId(funcId);
    Token* token = *tokens;
    Assignment assignment = { 0 };
    assert(token->type != tt_eof || token->type != tt_endl);

    u64 maxLeftSideElements = 8;
    u64 numLeftSideElements = 0;
    AssignmentLeftSideElement leftSideElements[maxLeftSideElements];

    while (true) {
        AssignmentLeftSideElement ase = createAssignmentLeftSideElementFromTokens(&token, funcId, scope, project);
        if (ase.type == ase_error) {
            assignment.leftSideElements = NULL;  // Clear variable name on error
            return assignment;
        }
        if (numLeftSideElements >= maxLeftSideElements) {
            logErrorToken("Too many variables being assigned to", project, token);
            assignment.leftSideElements = NULL;
            return assignment;
        }
        leftSideElements[numLeftSideElements] = ase;
        numLeftSideElements++;
        if (token->type == tt_endl) {
            break;
        }
        if (token->type == tt_assign || token->type == tt_add_assign || token->type == tt_sub_assign || token->type == tt_mul_assign || token->type == tt_div_assign || token->type == tt_mod_assign) {
            break;
        }
        if (token->type != tt_comma) {
            logErrorToken("Expected comma or end of line or equal sign", project, token);
            assignment.leftSideElements = NULL;  // Clear variable name on error
            return assignment;
        }
        token++;
    }
    assignment.leftSideElements = arenaAlloc(project->arena, sizeof(AssignmentLeftSideElement) * numLeftSideElements);
    memcpy(assignment.leftSideElements, leftSideElements, sizeof(AssignmentLeftSideElement) * numLeftSideElements);
    assignment.numLeftSideElements = numLeftSideElements;
    if (token->type == tt_endl) {
        assignment.value = NULL;
        *tokens = token;
        return assignment;
    }
    Token* assignToken = token;
    OurTokenType assignOp = token->type;
    token++;

    Expresstion value = createExpresstionFromTokens(&token, tt_endl, funcId, scope, project);

    if (value.type == et_error) {
        assignment.leftSideElements = NULL;  // Clear variable name on error
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
            if (numLeftSideElements != 1) {
                logErrorToken("Can only use operators like += or -= on one variable", project, token);
                assignment.leftSideElements = NULL;  // Clear variable name on error
                return assignment;
            }
            if (leftSideElements[0].type != ase_variable) {
                logErrorToken("Can only call operators like += or -= on variables and not on declarations", project, token);
                assignment.leftSideElements = NULL;  // Clear variable name on error
                return assignment;
            }
            Expresstion biop;
            BiopExpresstion biopExp;
            Expresstion left;
            left.type = et_variable;
            left.tid = leftSideElements[0].variable.type;
            left.variable = arenaAlloc(project->arena, strlen(leftSideElements[0].variable.name) + 1);
            memcpy(left.variable, leftSideElements[0].variable.name, strlen(leftSideElements[0].variable.name) + 1);
            biopExp.left = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(biopExp.left, &left, sizeof(Expresstion));
            biopExp.operator= assignOpToBiop(assignOp);
            biopExp.right = arenaAlloc(project->arena, sizeof(Expresstion));
            memcpy(biopExp.right, &value, sizeof(Expresstion));
            biop.type = et_biop;
            biop.tid = getBiopTypeResult(&biopExp, project);
            if (biop.tid == 0) {
                logErrorToken("Can't use assignment operator between types", project, assignToken);
                assignment.leftSideElements = NULL;  // Clear variable name on error
                return assignment;
            }
            biop.biopExpresstion = arenaAlloc(project->arena, sizeof(BiopExpresstion));
            memcpy(biop.biopExpresstion, &biopExp, sizeof(BiopExpresstion));
            value = biop;
            break;
        }
        default:
            logErrorToken("Invalid assignment operator", project, token);
            assignment.leftSideElements = NULL;  // Clear variable name on error
            return assignment;
    }

    Type* rightType = getTypeFromId(value.tid);
    bool rightTypeisStruct = rightType->kind == tk_struct;
    if (numLeftSideElements == 1) {
        AssignmentLeftSideElement* ase = &leftSideElements[0];
        typeId leftType = ase->variable.type;
        bool implCast = canImplCast(value.tid, leftType, project);
        if (!implCast) {
            logErrorToken("Type mismatch in assignment", project, token);
            assignment.leftSideElements = NULL;  // Clear variable name on error
            return assignment;
        }
    } else if (rightTypeisStruct) {
        u64 rightTypeStructNumFields = rightType->structVals.memberCount;
        if (rightTypeStructNumFields != numLeftSideElements) {
            logErrorToken("Can't assign decompose struct into variables when number of variables does not match number of fields in struct", project, token);
            assignment.leftSideElements = NULL;  // Clear variable name on error
            return assignment;
        }
        for (u64 i = 0; i < numLeftSideElements; i++) {
            AssignmentLeftSideElement* ase = &leftSideElements[i];
            typeId leftType = ase->variable.type;
            bool implCast = canImplCast(value.tid, leftType, project);
            if (!implCast) {
                logErrorToken("Type mismatch in assignment", project, token);
                assignment.leftSideElements = NULL;  // Clear variable name on error
                return assignment;
            }
        }
    } else {
        logErrorToken("Can't assign decompose struct when right hand side is not a struct", project, token);
        assignment.leftSideElements = NULL;  // Clear variable name on error
        return assignment;
    }
    for (u64 i = 0; i < numLeftSideElements; i++) {
        AssignmentLeftSideElement* ase = &leftSideElements[i];
        typeId leftType = ase->variable.type;
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
                Assignment assignment = createAssignmentFromTokens(&token, funcId, scope, project);
                if (assignment.leftSideElements == NULL) {
                    return statement;  // Error in declaration
                }
                statement.type = st_assignment;
                statement.assignment = arenaAlloc(project->arena, sizeof(Assignment));
                memcpy(statement.assignment, &assignment, sizeof(Assignment));
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
                if (assignment.leftSideElements == NULL) {
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
