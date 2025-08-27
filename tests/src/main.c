#include "span_parser.h"
#include "span_parser/arena.h"
#include "span_parser/utils.h"

//#define SAVE_TEST_RESULTS

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

    Arena directoryArena = createRootArena(1024);
    u64 directoryCount;
    char** directories = getDirectoryNamesInDirectory(directoryArena, pathToSpanTests, &directoryCount);

    char* exeName = "build/output.exe";

    bool resBuffer[BUFFER_SIZE];

    for (u64 i = 0; i < directoryCount; i++) {
        bool testPassed = true;
        // skip scrach as that is just me testing out new stuff
        char* directoryName = directories[i];
        if (strcmp(directoryName, "scratch") == 0) {
            continue;
        }

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
            char buffer[BUFFER_SIZE * 8];
            res = runExe(exeName, buffer);
            printf("%s", buffer);
#ifdef SAVE_TEST_RESULTS
            char coutFilePath[BUFFER_SIZE];
            sprintf(coutFilePath, "%s/test_stdout.txt", directoryPath);
            FILE* file = fopen(coutFilePath, "w");
            if (file == NULL) {
                redprintf("failed to open file\n");
                return -1;
            }
            fprintf(file, "%s", buffer);
            fclose(file);

            char returnCodeFilePath[BUFFER_SIZE];
            sprintf(returnCodeFilePath, "%s/test_return_code.txt", directoryPath);
            FILE* file2 = fopen(returnCodeFilePath, "w");
            if (file2 == NULL) {
                redprintf("failed to open file\n");
                return -1;
            }
            fprintf(file2, "%d", res);
            fclose(file2);
#else
            // check if we match the expected output
            printBar();
            char coutFilePath[BUFFER_SIZE];
            sprintf(coutFilePath, "%s/test_stdout.txt", directoryPath);
            char* coutFileContents = readFile(context.arena, coutFilePath);
            if (coutFileContents == NULL) {
                redprintf("failed to read file\n");
            } else {
                if (strcmp(coutFileContents, buffer) != 0) {
                    redprintf("cout different than expected\n");
                    testPassed = false;
                } else {
                    greenprintf("cout same as expected\n");
                }
            }

            char returnCodeFilePath[BUFFER_SIZE];
            sprintf(returnCodeFilePath, "%s/test_return_code.txt", directoryPath);
            char* returnCodeFileContents = readFile(context.arena, returnCodeFilePath);
            if (returnCodeFileContents == NULL) {
                redprintf("failed to read file\n");
            } else {
                if (atoi(returnCodeFileContents) != res) {
                    redprintf("return code different than expected\n");
                    testPassed = false;
                } else {
                    greenprintf("return code same as expected\n");
                }
            }
#endif
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

        if (context.numberOfErrors != 0) {
            testPassed = false;
        }

        if (testPassed) {
            greenprintf("test %s passed\n", directories[i]);
            resBuffer[i] = true;
        } else {
            redprintf("test %s failed\n", directories[i]);
            resBuffer[i] = false;
        }

        cleanupSpanContext();
        arenaReset(arena);
    }


    printf("\n");
    printBar();
    printf("test results\n");
    printBar();
    printf("\n");
    // print results
    for (u64 i = 0; i < directoryCount; i++) {
        char* directoryName = directories[i];
        if (strcmp(directoryName, "scratch") == 0) {
            continue;
        }
        if (resBuffer[i]) {
            greenprintf("test %s passed\n", directories[i]);
        } else {
            redprintf("test %s failed\n", directories[i]);
        }
    }
    printf("\n");
    printBar();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n");


    freeArena(arena);
    freeArena(directoryArena);
    debugbreak();
    return 0;
}
