#include "parser.h"
#include <string.h>

char* addStrings(const char* a, const char* b, Arena* arena) {
    i64 lenA = strlen(a);
    i64 lenB = strlen(b);
    i64 lenLargest = lenA > lenB ? lenA : lenB;
    char* res = arenaAlloc(arena, lenLargest + 2);
    res[lenLargest + 1] = '\0';
    i64 carry = 0;
    for (u64 i = 0; i < lenLargest; i++) {
        i64 iA = lenA - i - 1;
        i64 iB = lenB - i - 1;
        if (iA >= 0 && iB >= 0) {
            i64 addValue = a[iA] - '0' + b[iB] - '0' + carry;
            i64 digit = addValue % 10;
            carry = addValue / 10;
            res[lenLargest - i] = digit + '0';
        } else if (iA >= 0) {
            i64 digit = a[iA] - '0' + carry;
            carry = 0;
            res[lenLargest - i] = digit + '0';
        } else if (iB >= 0) {
            i64 digit = b[iB] - '0' + carry;
            carry = 0;
            res[lenLargest - i] = digit + '0';
        }
    }
    if (carry == 1) {
        res[0] = '1';
        return res;
    } else {
        return res + 1;
    }
}

char* subStrings(const char* a, const char* b, Arena* arena) {
    assert(false && "Not implemented");
    return NULL;
}

char* mulStrings(const char* a, const char* b, Arena* arena) {
    assert(false && "Not implemented");
    return NULL;
}


char* divStrings(const char* a, const char* b, Arena* arena) {
    assert(false && "Not implemented");
    return NULL;
}

char* modStrings(const char* a, const char* b, Arena* arena) {
    assert(false && "Not implemented");
    return NULL;
}

int compareStrings(const char* a, const char* b) {
    u64 lenA = strlen(a);
    u64 lenB = strlen(b);
    if (lenA == lenB) {
        return strcmp(a, b);
    }
    if (lenA > lenB) {
        return 1;
    }
    return -1;
}

char* twoToThePowerOf(u64 numBits, Arena* arena) {
    char* res = arenaAlloc(arena, numBits + 2);  // Enough for binary string + '\0'
}
