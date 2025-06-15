#pragma once
#include "nice_ints.h"
#include "parser/arena.h"

typedef struct _keyValue {
    char* key;
    void* value;
} keyValue;

typedef struct _keyValueList {
    keyValue* keyValue;
    u32 count;
    u32 capacity;
} keyValueList;

typedef struct _map {
    u64 capacity;
    Arena* arena;
    keyValueList* keyVals;
} map;


map createMap();
map createMapCapacity(Arena* arena, u64 capacity);
map createMapArena(Arena* arena);
map createMapArenaCapacity(Arena* arena, u64 capacity);


// make sure this is not called if you gave your own arena
void mapFree(map* map);

void mapSet(map* map, char* key, void* value);
void** mapGet(map* map, const char* key);
