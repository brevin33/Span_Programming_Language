#include "parser.h"
#include "string.h"
#include "stdlib.h"
#include <stdio.h>

int main() {
    initParser();

    const char* variableValue = getenv("Span_Language_Dir");
    assert(variableValue != NULL && "Environment variable 'Span_Language_Dir' is not set");
    char projectDir[512];
    sprintf(projectDir, "%s/span_examples/basic", variableValue);
    projectId projectId = createProject(projectDir);
    Project* project = getProjectFromId(projectId);
    printf("Project created with directory: %s\n", project->directory);

    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        Token* tokens = sourceCode->tokens;
        printf("Tokens in project:\n");
        while (tokens->type != tt_eof) {
            char buffer[256];

            tokenToString(tokens, buffer, sizeof(buffer));
            printf("%s | ", buffer);
            if (tokens->type == tt_endl) {
                printf("\n");
            }
            tokens++;
        }
    }
    return 0;
}
