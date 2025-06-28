#include "parser.h"
#include "parser/map.h"

Scope globalScope;

void initParser() {
    gArena = createArena(1024 * 1024);
    projectPool = createPool(sizeof(Project), gArena);
    sourceCodePool = createPool(sizeof(SourceCode), gArena);
    typeMap = createMapArenaCapacity(gArena, 1024 * 128);
    typePool = createPool(sizeof(Type), gArena);
    functionMap = createMapArenaCapacity(gArena, 1024 * 128);
    functionPool = createPool(sizeof(Function), gArena);
    globalScope = createScope(BAD_ID, gArena);
    setupDefaultTypes();
}
