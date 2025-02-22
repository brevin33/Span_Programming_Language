#include "span.h"
#include <filesystem>
#include <fstream>
#include <assert.h>



LLVMContextRef context;
LLVMBuilderRef builder;
Module* baseModule;
unordered_map<string, vector<Type>> nameToType;
unordered_map<string, vector<Function>> nameToFunction;
Module* activeModule;


void setupBasicTypes() {
    // number types
    for (int i = 1; i <= 512; i++) {
        string uname = "u" + to_string(i);
        string iname = "i" + to_string(i);
        nameToType[uname].push_back(Type(LLVMIntType(i), uname, baseModule));
        nameToType[iname].push_back(Type(LLVMIntType(i), iname, baseModule));
    }
    nameToType["void"].push_back(Type(LLVMVoidType(), "void", baseModule));

    nameToType["int"].push_back(Type(LLVMIntType(32), "i32", baseModule));
    nameToType["uint"].push_back(Type(LLVMIntType(32), "u32", baseModule));

    nameToType["f16"].push_back(Type(LLVMHalfType(), "f16", baseModule));
    nameToType["f32"].push_back(Type(LLVMFloatType(), "f32", baseModule));
    nameToType["f64"].push_back(Type(LLVMDoubleType(), "f64", baseModule));

    nameToType["half"].push_back(Type(LLVMHalfType(), "f16", baseModule));
    nameToType["float"].push_back(Type(LLVMFloatType(), "f32", baseModule));
    nameToType["double"].push_back(Type(LLVMDoubleType(), "f64", baseModule));

    nameToType["char"].push_back(Type(LLVMIntType(8), "u8", baseModule));
}



void compile(const std::string& dir) {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();

    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
    baseModule = new Module("base");
    setupBasicTypes();

    Module module(dir);
    module.loadTokens();
    module.findStarts();
    module.setupTypesAndFunctions();

    bool err = module.printResult();

    cout << endl;
    char *ir = LLVMPrintModuleToString(module.llvmModule);
    printf("%s\n", ir);
    LLVMDisposeMessage(ir);  // Free the allocated string

    if (err) return;

    error_code ec;
    fs::remove_all("Build", ec);

    module.compileToObjFile("Build");

    vector<string> objFiles;
    for (const auto& entry : fs::directory_iterator("Build")) {
        fs::path path = entry.path();
        objFiles.push_back(path.string());
    }
    std::stringstream command;
    // new: try doing this instead lld-link mycode.obj /out:myprogram.exe /subsystem:console /defaultlib:libcmt
    command << "lld-link ";
    for (const auto& file : objFiles) {
        command << file << " ";
    }
    command << "/out:main.exe /subsystem:console /defaultlib:libcmt"; 
    int result = std::system(command.str().c_str());
    if (result == 0) {
        std::cout << "Linking successful!" << std::endl;
    } else {
        std::cout << "Linking failed with error code: " << result << std::endl;
    }
}
