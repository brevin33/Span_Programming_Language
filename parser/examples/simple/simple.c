#include "parser.h"
#include "string.h"
#include "stdlib.h"
#include <assert.h>
#include <stdio.h>

int main() {
    const char* variableValue = getenv("Span_Language_Dir");
    assert(variableValue != NULL && "Environment variable 'Span_Language_Dir' is not set");
    char projectDir[512];
    sprintf(projectDir, "%s/span_examples/hello_world", variableValue);
    Project project = createProject(projectDir);
    printf("Project created with directory: %s\n", project.directory);

    Token* tokens = project.tokens;
    printf("Tokens in project:\n");
    while (tokens->type != tt_eop) {
        char buffer[256];
        tokenToString(tokens, buffer, sizeof(buffer));
        printf("%s | ", buffer);
        if (tokens->type == tt_endl) {
            printf("\n");
        }
        tokens++;
    }
    return 0;
}
