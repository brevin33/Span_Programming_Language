#pragma once
#include "span_parser/default.h"
#include "span_parser/arena.h"
#include "span_parser/tokens.h"

typedef struct _SpanAst SpanAst;

typedef enum _SpanASTType : u8 {
    ast_invalid = 0,
    ast_file,
    ast_struct,
    ast_struct_field,
    ast_type,
    ast_tmod_ptr,
    ast_tmod_ref,
    ast_tmod_uptr,
    ast_tmod_sptr,
    ast_tmod_array,
    ast_tmod_list,
    ast_tmod_slice,
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

typedef struct _SpanAstType {
    char* name;
    SpanAst* mods;
    u32 modsCount;
    u32 modsCapacity;
} SpanAstType;

typedef struct _SpanAstFile {
    SpanAst* globalStatements;
    u32 globalStatementsCount;
    u32 globalStatementsCapacity;
} SpanAstFile;

typedef struct _SpanAstStruct {
    char* name;
    SpanAst* fields;
    u32 fieldsCount;
    u32 fieldsCapacity;
} SpanAstStruct;

typedef struct _SpanAstStructField {
    char* name;
    SpanAst* type;
} SpanAstStructField;

typedef struct _SpanAst {
    SpanASTType type;
    u32 tokenLength;
    Token* token;
    union {
        // anything larger than 64 bits should be a pointer
        SpanAstFile* file;
        SpanAstStruct* struct_;
        SpanAstStructField* structField;
        SpanAstType* type_;
        SpanAstTmodPtr tmodPtr;
        SpanAstTmodRef tmodRef;
        SpanAstTmodUptr tmodUptr;
        SpanAstTmodSptr tmodSptr;
        SpanAstTmodArray tmodArray;
        SpanAstTmodList tmodList;
        SpanAstTmodSlice tmodSlice;
    };
} SpanAst;

SpanAst AstGeneralIdParse(Arena arena, Token** tokens);
SpanAst createAst(Arena arena, Token** tokens);
SpanAst AstGeneralParse(Arena arena, Token** tokens);
SpanAst AstStructFieldParse(Arena arena, Token** tokens);
SpanAst AstStructParse(Arena arena, Token** tokens);
SpanAst AstTypeParse(Arena arena, Token** tokens);
SpanAst AstTmodParse(Arena arena, Token** tokens);
SpanAst AstTmodPtrParse(Arena arena, Token** tokens);
SpanAst AstTmodRefParse(Arena arena, Token** tokens);
SpanAst AstTmodUptrParse(Arena arena, Token** tokens);
SpanAst AstTmodSptrParse(Arena arena, Token** tokens);
SpanAst AstTmodListLikeParse(Arena arena, Token** tokens);

bool looksLikeFunctionDeclaration(Token** tokens);
bool looksLikeType(Token** tokens);
