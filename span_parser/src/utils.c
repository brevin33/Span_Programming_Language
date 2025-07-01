
#include "span_parser.h"

char* uintToString(u64 number, char* buffer) {
    u64 i = 0;
    u64 n = number;
    u64 digits = countDigits(n);

    buffer[digits] = '\0';
    for (i = digits; i > 0; i--) {
        buffer[i - 1] = '0' + (n % 10);
        n /= 10;
    }
    return buffer;
}

char* intToString(i64 number, char* buffer) {
    u64 abs_number;
    if (number < 0) {
        buffer[0] = '-';
        abs_number = (u64)(-number);
        uintToString(abs_number, buffer + 1);
        return buffer;
    } else {
        abs_number = (u64)number;
        uintToString(abs_number, buffer);
        return buffer;
    }
}

u64 countDigits(u64 n) {
    u64 digits = 0;
    do {
        digits++;
        n /= 10;
    } while (n != 0);
    return digits;
}

char* getDirectoryNameFromPath(char* path, char* buffer) {
    u64 pathLength = strlen(path);
    u64 directoryNameStart = 0;

    bool seenAnythingButSlash = false;
    for (u64 i = pathLength - 1; i >= 0; i--) {
        if (path[i] == '\\' || path[i] == '/') {
            if (!seenAnythingButSlash) continue;
            directoryNameStart = i + 1;
            break;
        } else {
            seenAnythingButSlash = true;
        }
    }

    memcpy(buffer, path + directoryNameStart, pathLength - directoryNameStart);
    buffer[pathLength - directoryNameStart] = '\0';
    return buffer;
}
