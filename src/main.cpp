#include <iostream>
#include <chrono>
#include "language.h"
using namespace std;

void main() {
    auto start = std::chrono::high_resolution_clock::now();

    SpanProgram program("../../../span_examples/simple");

    auto end = std::chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    cout << "Time: " << duration.count() << " seconds" << endl;
}