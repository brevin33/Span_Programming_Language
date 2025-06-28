#pragma once
#include "parser/nice_ints.h"
#include "parser/arena.h"

bool numberFitsInInt(u64 bits, char* number);
bool numberFitsInUInt(u64 bits, char* number);

typedef u64 typeId;
typedef struct _Expression Expression;

bool constExpressionNumberWorksWithType(Expression* expr, typeId tid, Arena* arena);
