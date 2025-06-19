#include "parser.h"

void initParser() {
    gArena = createArena(1024 * 1024);
    projectPool = createPool(sizeof(Project), gArena);
    sourceCodePool = createPool(sizeof(SourceCode), gArena);
    typeMap = createMapArena(gArena);
    typePool = createPool(sizeof(Type), gArena);
}
