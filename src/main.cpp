#include "span.h"
#include <chrono>
#include <thread>
int main() {
    auto start = std::chrono::high_resolution_clock::now();
    compile("C:/Users/brevi/dev/Span_Programming_Language/span_examples/loops");
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> duration = end - start;
    std::cout << "Time: " << duration.count() / 1000000000 << " seconds" << std::endl;

    std::cout << "attempting to run:" << std::endl << std::endl;
    int errorCode = std::system("main.exe");
    std::cout << std::endl << std::endl;
    std::cout << "Error Code: " << errorCode << std::endl;
    return 0;
}
