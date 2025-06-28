#include <stdio.h>
#include "compiler.h"
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/timeb.h>

int main() {
    initParser();

    struct _timeb parse_start, parse_end, compile_start, compile_end;

    _ftime_s(&parse_start);

    const char* variableValue = getenv("Span_Language_Dir");
    assert(variableValue != NULL && "Environment variable 'Span_Language_Dir' is not set");
    char projectDir[512];
    sprintf(projectDir, "%s/span_examples/basic", variableValue);
    projectId projectId = createProject(projectDir);
    Project* project = getProjectFromId(projectId);
    printf("Project created with directory: %s\n", project->directory);

    _ftime_s(&parse_end);
    double parse_ms = (parse_end.time - parse_start.time) * 1000.0 + (parse_end.millitm - parse_start.millitm);

    printf("\nParsing took: %.3f sec\n", parse_ms / 1000.0);

    printf("\nCompiling...\n\n");

    _ftime_s(&compile_start);
    compile(projectId);
    _ftime_s(&compile_end);
    double compile_ms = (compile_end.time - compile_start.time) * 1000.0 + (compile_end.millitm - compile_start.millitm);
    printf("Compiling took: %.3f sec\n", compile_ms / 1000.0);

    return 0;
}
