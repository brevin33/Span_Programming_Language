#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"

typedef struct _SpanAst SpanAst;

typedef enum programAction {
    pa_return,
    pa_break,
    pa_continue,
} ProgramAction;

typedef enum _SpanASTType : u8 {
    ast_invalid = 0,
    ast_file,
    ast_struct,
    ast_type,
    ast_tmod_ptr,
    ast_tmod_ref,
    ast_tmod_uptr,
    ast_tmod_sptr,
    ast_tmod_array,
    ast_tmod_list,
    ast_tmod_slice,
    ast_variable_declaration,
    ast_parameter_delcaration,
    ast_func_param,
    ast_scope,
    ast_expr_word,
    ast_assignment,
} SpanASTType;

typedef struct _SpanAstTmodPtr {
} SpanAstTmodPtr;

typedef struct _SpanAstTmodRef {
} SpanAstTmodRef;

typedef struct _SpanAstTmodUptr {
} SpanAstTmodUptr;

typedef struct _SpanAstTmodSptr {
} SpanAstTmodSptr;

typedef struct _SpanAstTmodArray {
    u64 size;
} SpanAstTmodArray;

typedef struct _SpanAstTmodList {
} SpanAstTmodList;

typedef struct _SpanAstTmodSlice {
} SpanAstTmodSlice;

typedef struct _SpanAstFunctionParameterDeclaration {
    SpanAst* params;
    u64 paramsCount;
} SpanAstFunctionParameterDeclaration;

typedef struct _SpanAstExprWord {
    char* word;
} SpanAstExprWord;

typedef struct _SpanAstFunctionDeclaration {
    char* name;
    SpanAst* returnType;
    SpanAst* paramList;
    SpanAst* body;
} SpanAstFunctionDeclaration;

typedef struct _SpanAstType {
    char* name;
    SpanAst* mods;
    u64 modsCount;
} SpanAstType;

typedef struct _SpanAstFile {
    SpanAst* globalStatements;
    u64 globalStatementsCount;
} SpanAstFile;

typedef struct _SpanAstStruct {
    char* name;
    SpanAst* fields;
    u64 fieldsCount;
    SpanAst* body;
} SpanAstStruct;

typedef struct _SpanAstVariableDeclaration {
    char* name;
    SpanAst* type;
} SpanAstVariableDeclaration;

typedef struct _SpanAstScope {
    SpanAst* statements;
    u64 statementsCount;
} SpanAstScope;


typedef struct _SpanAstAssignment {
    SpanAst* assignees;
    u64 assigneesCount;
    SpanAst* value;
} SpanAstAssignment;

typedef struct _SpanAst {
    SpanASTType type;
    u32 tokenLength;
    Token* token;
    union {
        // anything larger than 64 bits should be a pointer
        SpanAstFile* file;
        SpanAstStruct* struct_;
        SpanAstType* type_;
        SpanAstTmodPtr tmodPtr;
        SpanAstTmodRef tmodRef;
        SpanAstTmodUptr tmodUptr;
        SpanAstTmodSptr tmodSptr;
        SpanAstTmodArray tmodArray;
        SpanAstTmodList tmodList;
        SpanAstTmodSlice tmodSlice;
        SpanAstFunctionDeclaration* functionDeclaration;
        SpanAstVariableDeclaration* variableDeclaration;
        SpanAstFunctionParameterDeclaration* funcParam;
        SpanAstScope* scope;
        SpanAstAssignment* assignment;
        SpanAstExprWord* exprWord;
    };
} SpanAst;

SpanAst AstGeneralIdParse(Arena arena, Token** tokens);
SpanAst createAst(Arena arena, Token** tokens);
SpanAst AstGeneralParse(Arena arena, Token** tokens);
SpanAst AstStructParse(Arena arena, Token** tokens);
SpanAst AstTypeParse(Arena arena, Token** tokens);
SpanAst AstTmodParse(Arena arena, Token** tokens);
SpanAst AstTmodPtrParse(Arena arena, Token** tokens);
SpanAst AstTmodRefParse(Arena arena, Token** tokens);
SpanAst AstTmodUptrParse(Arena arena, Token** tokens);
SpanAst AstTmodSptrParse(Arena arena, Token** tokens);
SpanAst AstTmodListLikeParse(Arena arena, Token** tokens);
SpanAst AstFunctionDeclarationParse(Arena arena, Token** tokens);
SpanAst AstVariableDeclarationParse(Arena arena, Token** tokens);
SpanAst AstFunctionParameterDeclarationParse(Arena arena, Token** tokens);
SpanAst AstScopeParse(Arena arena, Token** tokens);
SpanAst AstExpressionParse(Arena arena, Token** tokens, OurTokenType* delimeters, u64 delimetersCount);
SpanAst AstAssignmentParse(Arena arena, Token** tokens);
SpanAst AstExpressionValueParse(Arena arena, Token** tokens);
SpanAst AstExpressionBiopParse(Arena arena, Token** tokens, i64 precedence, OurTokenType* delimeters, u64 delimetersCount);



bool looksLikeFunctionDeclaration(Token** tokens);
bool looksLikeType(Token** tokens);
