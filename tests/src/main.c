#include <stdio.h>
#include "span_parser.h"
#include "span_parser/logging.h"

int main(void) {
    const char* variableValue = getenv("Span_Language_Dir");
    char pathToSpanTests[4096];
    sprintf(pathToSpanTests, "%s/tests/span_tests", variableValue);
    Arena arena = createRootArena(1024 * 1024 * 4);

    u64 directoryCount;
    char** directories = getDirectoryNamesInDirectory(arena, pathToSpanTests, &directoryCount);

    for (u64 i = 0; i < directoryCount; i++) {
        char directoryPath[4096];
        sprintf(directoryPath, "%s/%s", pathToSpanTests, directories[i]);
        printf("\n");
        printBar();
        printf("running test for %s:\n", directories[i]);
        printBar();
        printf("\n");
        SpanProject project = createSpanProject(arena, directoryPath);
        printf("\n");
        printBar();
        printf("test done for %s\n", directories[i]);
        printBar();
    }

    return 0;
}
