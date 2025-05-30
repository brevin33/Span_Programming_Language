#include "parser.h"
#include "parser/tokens.h"

using namespace std;

void drawLine() {
    std::cout << "----------------------------------------" << std::endl;
}

void logError(std::string error) {
    std::cout << "\033[31m" << std::endl;
    std::cout << "Error: " << error << std::endl;
    std::cout << "\033[0m" << std::endl;
}

void logError(std::string error, Token token, Tokens& tokens) {
    std::cout << "\033[31m" << std::endl;
    std::cout << "Error in file " + tokens.filename + " on line " + to_string(token.line) + ": " << error << std::endl;
    std::cout << "\033[0m" << std::endl;
    std::cout << tokens.lines[token.line] << std::endl;
    for (u64 i = 0; i < token.charStart; i++) {
        std::cout << " ";
    }
    for (u64 i = token.charStart; i < token.charEnd; i++) {
        std::cout << "^";
    }
    std::cout << std::endl;
}
