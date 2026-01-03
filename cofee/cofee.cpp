#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cctype>

namespace fs = std::filesystem;

bool isLineEmpty(const std::string& line)
{
    for (char c : line)
    {
        if (!std::isspace(static_cast<unsigned char>(c)))
        {
            return false; 
        }
    }
    return true; 
}

bool isCodeFile(const fs::path& filePath)
{
    std::string ext = filePath.extension().string();
    return (ext == ".cpp" || ext == ".h" || ext == ".hpp" ||
        ext == ".c" || ext == ".java" || ext == ".py");
}

int main()
{
    std::string path = "E:\\V33\\js";
    int totalLines = 0;
    int nonEmptyLines = 0;
    int fileCount = 0;

    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file() && isCodeFile(entry.path()))
        {
            std::string currentPath = entry.path().string();
            std::ifstream fileReader(currentPath);

            if (!fileReader.is_open())
            {
                std::cerr << "Could not open: " << currentPath << "\n";
                continue;
            }

            std::string line;
            while (std::getline(fileReader, line))
            {
                totalLines++;

                if (!isLineEmpty(line))
                {
                    nonEmptyLines++;
                }
            }

            fileReader.close();
            fileCount++;
            std::cout << "Scanned: " << currentPath << "\n";
        }
    }

    std::cout << "--------------------------------\n";
    std::cout << "Total Files: " << fileCount << "\n";
    std::cout << "Total Lines: " << totalLines << "\n";
    std::cout << "Non-Empty Lines: " << nonEmptyLines << "\n";
    std::cout << "Empty/Whitespace Lines: " << (totalLines - nonEmptyLines) << "\n";

    return 0;
}