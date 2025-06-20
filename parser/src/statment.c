#include "parser.h"


Statement createStatmentFromTokens(Token** tokens, functionId functionId, Scope* scope) {
    Token* token = *tokens;
    Statement statement = { 0 };

    while (token->type == tt_endl) {
        token++;
    }

    Token* start = token;
    switch (token->type) {
        case tt_id: {
            typeId type = getTypeIdFromTokens(&token);
            if (type != BAD_ID) {
                // type
                if (token->type == tt_dot) { }
            }
            // not type
        }
        default: {
            logErrorTokens(token, 1, "Can't understand statement");
            while (token->type != tt_endl) {
                token++;
            }
        }
    }

    *tokens = token;
}

Statement createExpressionStatement(Token** tokens, functionId functionId, Scope* scope) {
    Statement statement = { 0 };
    Token* token = *tokens;



    *tokens = token;
    statement.kind = sk_expression;
    return statement;
}

Statement createAssignmentStatement(Token** tokens, functionId functionId, Scope* scope) {
}
