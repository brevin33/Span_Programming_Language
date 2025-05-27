#pragma once
#include "nice_ints.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>  // for std::move


struct Arena {
    explicit Arena(u64 initialSize)
        : capacity(initialSize)
        , startIndex(0) {
        start = new void*[MaxBlocks];
        std::memset(start, 0, MaxBlocks * sizeof(void*));
        start[0] = ::operator new(capacity);
        current = start[0];
    }

    ~Arena() {
        reset();
    }

    // Move constructor
    Arena(Arena&& other) noexcept
        : start(other.start)
        , startIndex(other.startIndex)
        , capacity(other.capacity)
        , current(other.current) {
        other.start = nullptr;
        other.startIndex = 0;
        other.capacity = 0;
        other.current = nullptr;
    }

    // Move assignment
    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            reset();  // free current resources if any

            start = other.start;
            startIndex = other.startIndex;
            capacity = other.capacity;
            current = other.current;

            other.start = nullptr;
            other.startIndex = 0;
            other.capacity = 0;
            other.current = nullptr;
        }
        return *this;
    }

    // Disable copy
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    void* alloc(u64 size) {
        size = (size + 7) & ~7ULL;  // Align to 8 bytes
        u8* base = static_cast<u8*>(start[startIndex]);
        u8* curr = static_cast<u8*>(current);
        if ((u64)(curr - base) + size > capacity) {
            ++startIndex;
            assert(startIndex < MaxBlocks);

            capacity *= 2;
            start[startIndex] = ::operator new(capacity);
            current = start[startIndex];
        }

        void* mem = current;
        current = static_cast<void*>(static_cast<u8*>(current) + size);
        return mem;
    }

    void reset() {
        for (u64 i = 0; i <= startIndex; ++i) {
            ::operator delete(start[i]);
        }
        delete[] start;
        start = nullptr;
        startIndex = 0;
        capacity = 0;
        current = nullptr;
    }

private:
    static constexpr u64 MaxBlocks = 128;
    void** start = nullptr;
    u64 startIndex = 0;
    u64 capacity = 0;
    void* current = nullptr;
};
