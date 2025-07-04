#include "tree_sitter/parser.h"
#include <wctype.h>

enum TokenType {
    END_OF_STATEMENT,
};

void *tree_sitter_span_external_scanner_create() {
    return NULL;
}
void tree_sitter_span_external_scanner_destroy(void *p) {
}
unsigned tree_sitter_span_external_scanner_serialize(void *p, char *b) {
    return 0;
}
void tree_sitter_span_external_scanner_deserialize(void *p, const char *b, unsigned n) {
}

bool tree_sitter_span_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    if (!valid_symbols[END_OF_STATEMENT]) return false;

    // Skip whitespace before braces
    while (iswspace(lexer->lookahead) && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
        lexer->advance(lexer, true);
    }
    // Check for comma or braces before newline
    if (lexer->lookahead == ',') {  //|| lexer->lookahead == '{' || lexer->lookahead == '}') {
        lexer->advance(lexer, false);
        // Skip whitespace after symbol
        while (iswspace(lexer->lookahead) && lexer->lookahead != '\n' && lexer->lookahead != '\r') {
            lexer->advance(lexer, true);
        }
        // If next is newline, do not treat as end of statement
        if (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
            return false;
        }
    }

    // Accept newline or semicolon as end of statement
    if (lexer->lookahead == '\n' || lexer->lookahead == '\r' || lexer->lookahead == ';') {
        lexer->advance(lexer, false);
        lexer->result_symbol = END_OF_STATEMENT;
        return true;
    }

    return false;
}
