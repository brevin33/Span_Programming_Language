#pragma once
#include "span_parser/default.h"

typedef enum _TypeKind : u8 {
    tk_invalid = 0,
} TypeKind;

typedef struct _Type {
    TypeKind kind;
} Type;
