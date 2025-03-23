#include "span.h"

CompilerContext context;

void logError(const string& err, Token token, bool wholeLine) {
    context.hadCompileError = true;
    std::cout << "\033[31m";
    cout << "Error: " << err << endl;
    std::cout << "\033[0m";
    cout << removeSpaces(context.textByFileByLine[token.file][token.line]) << endl;
    std::cout << "\033[31m";
    if (wholeLine) {
        for (int i = 0; i <= context.textByFileByLine[token.file][token.line].size(); i++) {
            cout << "^";
        }
    } else {
        bool startSpaces = true;
        for (int i = 0; i < token.schar; i++) {
            if (isspace(context.textByFileByLine[token.file][token.line][i]) && startSpaces) {
                continue;
            }
            cout << " ";
            startSpaces = false;
        }
        for (int i = token.schar; i <= token.echar; i++) {
            cout << "^";
        }
    }
    std::cout << "\033[0m";
    cout << endl;
    cout << "Line: " << token.line << " | File: " << context.files[token.file] << endl;
    cout << "-------------------------------------" << endl;
}

void compile(std::string dir) {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();
    context.llvmContext = LLVMContextCreate();
    context.llvmBuilder = LLVMCreateBuilderInContext(context.llvmContext);

    Module module(dir);
}
