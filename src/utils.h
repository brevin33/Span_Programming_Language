#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <inttypes.h>
#include <filesystem>
#include <cassert>
#include <optional>
#include <fstream>
using namespace std;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef float f32;
typedef double f64;

struct context { };
extern context g;


static void makeRed() {
    cout << "\033[31m";
}
static void makeGreen() {
    cout << "\033[32m";
}
static void makeYellow() {
    cout << "\033[33m";
}
static void makeBlue() {
    cout << "\033[34m";
}
static void makeMagenta() {
    cout << "\033[35m";
}
static void makeCyan() {
    cout << "\033[36m";
}
static void makeWhite() {
    cout << "\033[37m";
}
static void makeNormal() {
    cout << "\033[0m";
}
static void makeBold() {
    cout << "\033[1m";
}

static void printLine() {
    cout << "----------------------------------------" << endl;
}

static optional<u64> getNumber(const string& str) {
    istringstream iss(str);
    u64 value;
    iss >> value;
    if (iss.fail()) {
        return nullopt;
    }
    return value;
}

static optional<f64> getReal(const string& str) {
    istringstream iss(str);
    f64 value;
    iss >> value;
    if (iss.fail()) {
        return nullopt;
    }
    return value;
}

static void logInfo(const string& message) {
    makeBlue();
    cout << "Info: " << message << endl;
    makeNormal();
    printLine();
}

static void logError(const string& message) {
    makeRed();
    cout << "Error: " << message << endl;
    makeNormal();
    printLine();
}

static void logWarning(const string& message) {
    makeYellow();
    cout << "Warning: " << message << endl;
    makeNormal();
    printLine();
}
