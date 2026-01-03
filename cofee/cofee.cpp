#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <map> 
#include <iomanip>

namespace fs = std::filesystem;

struct LanguageStats {
    int fileCount = 0;
    int codeLines = 0;
};

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

void printReport(std::ostream& out, const std::string& path,
    const std::map<std::string, LanguageStats>& statsMap,
    int totalLines)
{
    out << "------------------------------------------------\n";
    out << "PROJECT SCAN REPORT: " << path << "\n";
    out << "------------------------------------------------\n";
    out << std::left << std::setw(15) << "TYPE"
        << std::setw(15) << "FILES"
        << std::setw(15) << "LINES (CODE)" << "\n";
    out << "------------------------------------------------\n";

    for (const auto& [ext, stat] : statsMap)
    {
        out << std::left << std::setw(15) << ext
            << std::setw(15) << stat.fileCount
            << std::setw(15) << stat.codeLines << "\n";
    }
    out << "------------------------------------------------\n";
    out << "TOTAL REAL CODE: " << totalLines << "\n";
    out << "------------------------------------------------\n";
}

int main(int argc, char* argv[])
{
    std::string path = ".";
    bool generateReport = false;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--report" || arg == "-r")
        {
            generateReport = true;
        }
        else if (arg[0] != '-')
        {
            path = arg;
        }
    }

    if (!fs::exists(path) || !fs::is_directory(path))
    {
        std::cerr << "Error: The path '" << path << "' does not exist or is not a directory.\n";
        return 1;
    }

    std::map<std::string, LanguageStats> statsMap;
    int totalLines = 0;
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
            std::string ext = entry.path().extension().string();

            std::string line;
            bool inBlockComment = false;
            int fileRealLines = 0;

            while (std::getline(fileReader, line))
            {
                if (hasRealCode(line, inBlockComment))
                    fileRealLines++;
            }

            statsMap[ext].fileCount++;
            statsMap[ext].codeLines += fileRealLines;
            totalLines += fileRealLines;
            fileCount++;

            // progress indicator
            std::cout << "\r[Scanning] Files: " << fileCount
                << " | Lines: " << totalLines << "   " << std::flush;
        }
    }

    std::cout << "\r[Done] Scanned " << fileCount << " files.                                  \n";

    printReport(std::cout, path, statsMap, totalLines);

    if (generateReport)
    {
        std::ofstream reportFile("cofee_report.txt");
        if (reportFile.is_open())
        {
            printReport(reportFile, path, statsMap, totalLines);
            std::cout << "[Success] Report saved to 'cofee_report.txt'\n";
        }
        else
        {
            std::cerr << "[Error] Could not write report file.\n";
        }
    }


    return 0;
}