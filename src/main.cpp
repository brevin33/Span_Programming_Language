#include "span.h"
#include <chrono>
int main() {
    auto start = std::chrono::high_resolution_clock::now();
    compile("C:/Users/brevi/dev/Span_Programming_Language/span_examples/simple");
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> duration = end - start;
    std::cout << "Time: " << duration.count() / 1000000000 << " seconds" << std::endl;
    return 0;
}
