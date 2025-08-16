#include "span_parser.h"
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#ifdef _WIN32
    #include <windows.h>
double getTimeSeconds() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / freq.QuadPart;
}
#else
    #include <sys/time.h>
double getTimeSeconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
#endif

bool mkdir(const char* path) {
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "mkdir %s > NUL 2>&1", path);
    return system(buffer) == 0;
}

bool rmdir(const char* path) {
    char buffer[BUFFER_SIZE];
#if defined(_WIN32) || defined(_WIN64)
    sprintf(buffer, "rmdir /S /Q %s > NUL 2>&1", path);
#else
    sprintf(buffer, "rm -rf %s", path);
#endif
    return system(buffer) == 0;
}

void swapSlashes(char* path) {
    for (u64 i = 0; i < strlen(path); i++) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }
}

int runExe(char* exeName) {
    char buffer[BUFFER_SIZE];
#if defined(_WIN32) || defined(_WIN64)
    char name[BUFFER_SIZE];
    sprintf(name, "%s", exeName);
    swapSlashes(name);
    return system(name);
#else
    sprintf(buffer, "./%s", exeName);
    return system(exeName);
#endif
}

bool linkExe(char** objFiles, u64 objFilesCount, char* exeName) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    for (u64 i = 0; i < objFilesCount; i++) {
        sprintf(buffer, "%s %s", buffer, objFiles[i]);
    }
    char buffer2[BUFFER_SIZE];
    sprintf(buffer2, "lld-link %s /out:%s /subsystem:console /defaultlib:libcmt", buffer, exeName);
    bool worked = system(buffer2) == 0;
    return worked;
}

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

u64 stringToUint(char* string) {
    char* end;
    unsigned long long int value = strtoull(string, &end, 10);
    if (errno == ERANGE) {
        errno = 0;
        return 0;
    }
    return value;
}


bool stringIsUint(char* string) {
    char* end;
    unsigned long long int value = strtoull(string, &end, 10);
    if (errno == ERANGE) {
        errno = 0;
        return false;
    }
    return *end == '\0' && value <= ULLONG_MAX;
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
