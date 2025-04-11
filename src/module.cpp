#include "module.h"

Module::Module(string dir) {
    filesystem::path path = filesystem::path(dir);
    if (!filesystem::is_directory(path)) {
        logError("Module path is not a directory: " + dir);
        return;
    }
    name = path.filename().string();
    for (auto& p : filesystem::directory_iterator(dir)) {
        if (p.is_directory()) {
            dependecies.push_back(Module(p.path().string()));
        }
    }
    for (auto& p : filesystem::directory_iterator(dir)) {
        if (!p.is_regular_file()) {
            continue;
        }
        string ext = p.path().extension().string();
        if (ext != ".span") {
            continue;
        }
        string filename = p.path().filename().string();
        if (filename == "build.span") {
            // TODO: build rules
            abort();
            continue;
        }
        tokens.loadFile(p.path().string());
        tokens.print();
    }
}
