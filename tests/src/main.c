#include "span_parser.h"
#include "span_parser/utils.h"

int main(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    const char* variableValue = getenv("Span_Language_Dir");
    char pathToSpanTests[BUFFER_SIZE];
    sprintf(pathToSpanTests, "%s/tests/span_tests", variableValue);
    Arena arena = createRootArena(1024 * 1024 * 4);

    u64 directoryCount;
    char** directories = getDirectoryNamesInDirectory(arena, pathToSpanTests, &directoryCount);

    char* exeName = "build/output.exe";

    for (u64 i = 0; i < directoryCount; i++) {
        char directoryPath[BUFFER_SIZE];
        sprintf(directoryPath, "%s/%s", pathToSpanTests, directories[i]);
        printf("\n");
        printBar();
        printf("parsing project %s\n", directories[i]);
        printBar();
        printf("\n");

        double parse_start = getTimeSeconds();
        SpanProject project = createSpanProject(arena, directoryPath);
        double parse_end = getTimeSeconds();

        printf("\n\n");
        printf("Parsing time: %.6f seconds\n", parse_end - parse_start);

        printf("\n");
        printBar();
        printf("compiling project %s\n", directories[i]);
        printBar();
        printf("\n");

        double compile_start = getTimeSeconds();
        compileSpanProject(&project);
        double compile_end = getTimeSeconds();
        printf("\n\n");
        printf("Compilation time: %.6f seconds\n", compile_end - compile_start);

        printf("\n");
        printBar();
        printf("running project %s\n", directories[i]);
        printBar();
        printf("\n");

        double run_start = getTimeSeconds();
        int res = 0;
        if (context.numberOfErrors > 0) {
            redprintf("can't run project with errors\n");
        } else {
            res = runExe(exeName);
        }
        double run_end = getTimeSeconds();

        printf("\n\n");
        if (context.numberOfErrors == 0) {
            printf("program exit code is: %d\n", res);
        }
        printf("Execution time: %.6f seconds\n", run_end - run_start);
        printf("\n");
        printBar();
        printf("test done for %s\n", directories[i]);
        printBar();
    }

    freeArena(arena);
    return 0;
}
