#pragma once
#include "parser.h"
#include "compiler/llvm.h"
#include "compiler/project.h"
#include "compiler/function.h"
#include "compiler/type.h"
#include "compiler/scope.h"

void compileProject(Project* project);
