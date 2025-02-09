#include "span.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

string loadFileToString(const string& filePath)
{
    ifstream file(filePath, ios::in | ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open file " << filePath << '\n';
        return "";
    }
    auto fileSize = file.tellg();
    string content(fileSize, '\0');
    file.seekg(0);
    file.read(&content[0], fileSize);
    return content;
}

vector<string> splitStringByNewline(const std::string& str)
{
    vector<string> lines;
    istringstream stream(str);
    string line;
    lines.push_back("");

    while (std::getline(stream, line))
    {
        lines.push_back(line);
    }

    return lines;
}

void logError(const string& err, Token& token, Module& module)
{
    cout << "Error: " << err;
    cout << "In file " << module.files[token.file] << " On line " << token.line << endl;
    cout << endl;
    cout << module.textByFileByLine[token.file][token.line] << endl;
    for (int i = 0; i < token.schar; i++)
    {
        cout << " ";
    }
    for (int i = token.schar; i <= token.echar; i++)
    {
        cout << "^";
    }
    cout << endl;
    cout << endl;
    cout << "-------------------------------------" << endl;
}

void getTokens(u8 file, Module& module)
{
    module.tokensByFile.push_back({});
    vector<Token>& tokens = module.tokensByFile.back();
    for (u16 lineNum = 0; lineNum < module.textByFileByLine[file].size(); lineNum++)
    {
        string& line = module.textByFileByLine[file][lineNum];
        for (u16 c = 0; c < line.size(); c++)
        {
            while (c < line.size() && isspace(line[c]))
                c++;
            Token token;
            token.schar = c;
            token.line = lineNum;
            token.file = file;

            switch (line[c])
            {
            case '_':
            CASELETTER:
            {
                stringstream ss;
                bool done = false;
                while (c < line.size() && !done)
                {
                    switch (line[c])
                    {
                    case '_':
                    CASELETTER:
                    CASENUMBER:
                    {
                        ss << line[c];
                        c++;
                        break;
                    }
                    default:
                    {
                        done = true;
                        break;
                    }
                    }
                }
                c--;
                token.data.str = new string;
                *token.data.str = ss.str();
                token.echar = c;
                token.type = tt_id;
                break;
            }
            case '"':
            {
                c++;
                stringstream ss;
                bool done = false;
                while (c < line.size() && !done)
                {
                    switch (line[c])
                    {
                    case '"':
                    {
                        done = true;
                        break;
                    }
                    default:
                    {
                        ss << line[c];
                        c++;
                        break;
                    }
                    }
                }
                if (!done)
                {
                    token.echar = c;
                    token.type == tt_err;
                    break;
                }
                c--;
                token.data.str = new string;
                *token.data.str = ss.str();
                token.echar = c;
                token.type = tt_str;
                break;
            }
            case '.':
            CASENUMBER:
            {
                bool isFloat = false;
                stringstream ss;
                while (c < line.size() && (isdigit(line[c]) || line[c] == '.'))
                {
                    ss << line[c];
                    if (line[c] == '.') isFloat = true;
                    c++;
                }
                c--;
                token.echar = c;
                token.type = isFloat ? tt_float : tt_int;
                if (isFloat)
                {
                    token.data.dec = std::stod(ss.str());
                }
                else
                {
                    token.data.uint = std::stoull(ss.str());
                }
                break;
            }
            case '=':
            {
                if (c + 1 < line.size() && line[c + 1] == '=')
                {
                    c++;
                    token.type = tt_eqeq;
                    token.echar = c;
                    break;
                }
                token.type = tt_eq;
                token.echar = c;
                break;
            }
            case '|':
            {
                if (c + 1 < line.size() && line[c + 1] == '|')
                {
                    c++;
                    token.type = tt_oror;
                    token.echar = c;
                    break;
                }
                token.type = tt_or;
                token.echar = c;
                break;
            }
            case '&':
            {
                if (c + 1 < line.size() && line[c + 1] == '&')
                {
                    c++;
                    token.type = tt_andand;
                    token.echar = c;
                    break;
                }
                token.type = tt_and;
                token.echar = c;
                break;
            }
            case '<':
            {
                if (c + 1 < line.size() && line[c + 1] == '=')
                {
                    c++;
                    token.type = tt_leeq;
                    token.echar = c;
                    break;
                }
                token.type = tt_le;
                token.echar = c;
                break;
            }
            case '>':
            {
                if (c + 1 < line.size() && line[c + 1] == '=')
                {
                    c++;
                    token.type = tt_greq;
                    token.echar = c;
                    break;
                }
                token.type = tt_gr;
                token.echar = c;
                break;
            }
            case '+':
            {
                token.type = tt_add;
                token.echar = c;
                break;
            }
            case '-':
            {
                token.type = tt_sub;
                token.echar = c;
                break;
            }
            case '*':
            {
                token.type = tt_mul;
                token.echar = c;
                break;
            }
            case '/':
            {
                token.type = tt_div;
                token.echar = c;
                break;
            }
            case '(':
            {
                token.type = tt_lpar;
                token.echar = c;
                break;
            }
            case ')':
            {
                token.type = tt_rpar;
                token.echar = c;
                break;
            }
            case '{':
            {
                token.type = tt_lcur;
                token.echar = c;
                break;
            }
            case '}':
            {
                token.type = tt_rcur;
                token.echar = c;
                break;
            }
            case '[':
            {
                token.type = tt_lbar;
                token.echar = c;
                break;
            }
            case ']':
            {
                token.type = tt_rbar;
                token.echar = c;
                break;
            }
            case '^':
            {
                token.type = tt_car;
                token.echar = c;
                break;
            }
            default:
            {
                token.echar = c;
                token.type = tt_err;
            }
            }
            tokens.push_back(token);
        }
        Token t;
        t.line = lineNum;
        t.file = file;
        t.schar = line.size();
        t.echar = line.size();
        t.type = tt_endl;
        tokens.push_back(t);
    }
}

void compileModule(const string& dir)
{
    Module module;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        if (path.extension() != "span") continue;
        module.files.push_back(path.string());
        module.textByFileByLine.push_back(splitStringByNewline(loadFileToString(path.string())));
    }
}

void setupGenerics()
{
}

void compile(const std::string& dir)
{
    setupGenerics();
    compileModule(dir);
}
