#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "compiler.h"

int main() {
    const char* variableValue = getenv("Span_Language_Dir");
    assert(variableValue != NULL && "Environment variable 'Span_Language_Dir' is not set");
    char projectDir[512];
    sprintf(projectDir, "%s/span_examples/basic", variableValue);

    struct timespec start, end;
    timespec_get(&start, TIME_UTC);
    Project project = createProject(projectDir);
    timespec_get(&end, TIME_UTC);
    double createTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Project created with directory: %s\n", project.directory);

    timespec_get(&start, TIME_UTC);
    compileProject(&project);
    timespec_get(&end, TIME_UTC);
    double compileTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double totalTime = createTime + compileTime;
    printf("--------------------------------------------------------------------------------\n");
    printf("time to parse project: %.6f seconds\n", createTime);
    printf("time to compile project: %.6f seconds\n", compileTime);
    printf("total time: %.6f seconds\n", totalTime);

    return 0;
}
