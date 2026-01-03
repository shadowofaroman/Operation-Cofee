#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// extension checker
bool isCodeFile(const fs::path& filePath)
{
    std::string ext = filePath.extension().string();
    return (ext == ".cpp" || ext == ".h" || ext == ".hpp" ||
        ext == ".c" || ext == ".cs" ||
        ext == ".js" || ext == ".ts" ||
        ext == ".jsx" || ext == ".tsx" ||
        ext == ".css" || ext == ".scss" || ext == ".html" ||
        ext == ".vue" || ext == ".json");
}

bool hasRealCode(const std::string& line, bool& inBlockComment)
{
    bool foundCode = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        if (inBlockComment)
        {
            // check if we found the exit door "*/"
            if (i + 1 < line.length() && line[i] == '*' && line[i + 1] == '/')
            {
                inBlockComment = false; 
                i++; 
            }
            continue;
        }

        // check for start of /* block */
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '*')
        {
            inBlockComment = true; 
            i++; 
            continue;
        }

        // check for single line comment //
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '/')
        {
            break;
        }

        if (!std::isspace(static_cast<unsigned char>(line[i])))
        {
            foundCode = true;
        }
    }

    return foundCode;
}

int main()
{
    std::string path = "E:\\V33";

    int totalLines = 0;
    int realCodeLines = 0;
    int fileCount = 0;

    std::cout << "Scanning " << path << "...\n";

    for (auto it = fs::recursive_directory_iterator(path); it != fs::recursive_directory_iterator(); ++it)
    {
        const auto& entry = *it;
        std::string pathStr = entry.path().string();

        // ignore Logic
        if (pathStr.find("node_modules") != std::string::npos ||
            pathStr.find(".git") != std::string::npos ||
            pathStr.find("dist") != std::string::npos ||
            pathStr.find(".vs") != std::string::npos ||
            pathStr.find("build") != std::string::npos)
        {
            it.disable_recursion_pending();
            continue;
        }

        if (entry.is_regular_file() && isCodeFile(entry.path()))
        {
            std::ifstream fileReader(pathStr);
            if (!fileReader.is_open()) continue;

            std::string line;

            bool inBlockComment = false;

            while (std::getline(fileReader, line))
            {
                totalLines++;

                if (hasRealCode(line, inBlockComment))
                {
                    realCodeLines++;
                }
            }
            fileCount++;
        }
    }

    std::cout << "--------------------------------\n";
    std::cout << "Project: " << path << "\n";
    std::cout << "Total Files: " << fileCount << "\n";
    std::cout << "Raw Lines: " << totalLines << "\n";
    std::cout << "Real Code Lines: " << realCodeLines << "\n";
    std::cout << "Comment/Empty Lines: " << (totalLines - realCodeLines) << "\n";

    return 0;
}