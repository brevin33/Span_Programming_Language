#include "parser.h"
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

Project::Project(string folder)
    : folder(folder) {
    if (is_directory(folder)) {
        for (const auto& entry : directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".span") {
                this->tokens.emplace_back(entry.path().string());
            }
        }
    } else {
        logError("Project folder does not exist or is not a directory: " + folder);
        throw runtime_error("Project folder does not exist or is not a directory: " + folder);
    }
}
