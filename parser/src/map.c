#include "parser/map.h"
#include "parser.h"
#include "parser/arena.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

map createMapArenaCapacity(Arena* arena, u64 capacity) {
    map map;
    map.capacity = capacity;
    map.arena = arena;
    map.keyVals = arenaAlloc(arena, sizeof(keyValueList) * capacity);
    for (u64 i = 0; i < capacity; i++) {
        map.keyVals[i].capacity = 1;
        map.keyVals[i].keyValue = arenaAlloc(arena, sizeof(keyValue) * map.keyVals[i].capacity);
        map.keyVals[i].count = 0;
    }
    return map;
}

map crateMapCapacity(u64 capacity) {
    Arena* arenaPtr = createArena(1024 * capacity);
    return createMapArenaCapacity(arenaPtr, capacity);
}

static u64 _hashKey(const char* key, u64* outKeySize) {
    u64 hash = 0;
    u64 keySize = 0;
    for (const char* c = key; *c != '\0'; c++) {
        keySize++;
        hash += *c;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    *outKeySize = keySize;
    return hash;
}

map createMap() {
    Arena* arenaPtr = createArena(1024 * 1024);
    return createMapArena(arenaPtr);
}

map createMapArena(Arena* arena) {
    return createMapArenaCapacity(arena, 1024 * 512);
}

void mapFree(map* map) {
    arenaFree(map->arena);
}

void mapSet(map* map, char* key, void* value) {
    u64 keySize;
    u64 hash = _hashKey(key, &keySize);
    hash %= map->capacity;
    keyValueList* keyValueList = &map->keyVals[hash];
    if (keyValueList->count >= keyValueList->capacity) {
        keyValueList->capacity *= 2;
        void* newMemory = arenaAlloc(map->arena, sizeof(keyValue) * keyValueList->capacity);
        if (newMemory != NULL) {
            memcpy(newMemory, keyValueList->keyValue, sizeof(keyValue) * keyValueList->count);
        }
        keyValueList->keyValue = newMemory;
    }
    keyValueList->keyValue[keyValueList->count].key = arenaAlloc(map->arena, keySize + 1);
    memcpy(keyValueList->keyValue[keyValueList->count].key, key, keySize + 1);
    keyValueList->keyValue[keyValueList->count].value = value;
    keyValueList->count++;
}

void** mapGet(map* map, const char* key) {
    u64 keySize;
    u64 hash = _hashKey(key, &keySize);
    hash %= map->capacity;
    keyValueList* keyValueList = &map->keyVals[hash];
    for (u32 i = 0; i < keyValueList->count; i++) {
        if (strcmp(keyValueList->keyValue[i].key, key) == 0) {
            return &(keyValueList->keyValue[i].value);
        }
    }
    return NULL;
}
