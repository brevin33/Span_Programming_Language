#include <chrono>
#include <thread>
#include <iostream>
#include "span.h"

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    compile("../../../span_examples/simple");

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> duration = end - start;
    cout << "-------------------------------------" << endl;
    std::cout << "Time: " << duration.count() / 1000000000 << " seconds" << std::endl;

    std::cout << "attempting to run:" << std::endl;
    cout << "-------------------------------------" << endl;
    int errorCode = std::system("main.exe");
    cout << endl;
    cout << "-------------------------------------" << endl;
    std::cout << "Error Code: " << errorCode << std::endl;
    return 0;
}
