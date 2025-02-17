#include "utils.h"

string loadFileToString(const string& filePath) {
    ifstream file(filePath, ios::in | ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filePath << '\n';
        return "";
    }
    auto fileSize = file.tellg();
    string content(fileSize, '\0');
    file.seekg(0);
    file.read(&content[0], fileSize);
    return content;
}

vector<string> splitStringByNewline(const string& str) {
    vector<string> lines;
    istringstream stream(str);
    string line;
    lines.push_back("");

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
}

string removeSpaces(const string& str) {
    stringstream ss;
    bool startSpaces = true;
    for (int i = 0; i < str.size(); i++) {
        if (isspace(str[i]) && startSpaces) continue;
        ss << str[i];
        startSpaces = false;
    }
    return ss.str();
}
