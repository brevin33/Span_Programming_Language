#include "parser.h"
#include <cstdlib>
#include <iostream>
using namespace std;

int main() {
    const char* variableValue = getenv("Span_Language_Dir");
    string spanPath(variableValue);
    string projectDir = spanPath + "/span_examples/hello_world";
    cout << "Running for dir: " << projectDir << endl;
    cout << endl;
    Project project(projectDir);
    for (int i = 0; i < project.tokens.size(); i++) {
        cout << "Tokens for file: " << endl << to_string(project.tokens[i]) << endl;
        cout << endl;
    }
    return 0;
}
