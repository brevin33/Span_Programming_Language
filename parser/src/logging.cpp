#include "parser.h"

void logError(std::string error) {
    std::cout << "\033[31m" << std::endl;
    std::cout << "Error: " << error << std::endl;
    std::cout << "\033[0m" << std::endl;
}
