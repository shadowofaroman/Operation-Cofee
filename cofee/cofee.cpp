#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <map> 
#include <iomanip>
#include <limits>
#include <chrono>
#include <sstream>

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
    int totalLines, bool verbose, const std::string& maxFile, int maxLines,
    const std::string& minFile, int minLines)
{
    // Add timestamp to report
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&time));

    out << "================================================\n";
    out << "       COFEE - CODE COUNTER REPORT\n";
    out << "================================================\n";
    out << "Generated: " << timeStr << "\n";
    out << "Project:   " << path << "\n";
    out << "================================================\n\n";

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
    out << "TOTAL REAL CODE: " << totalLines << " lines\n";
    out << "================================================\n";

    if (verbose)
    {
        out << "\n[VERBOSE ANALYTICS]\n";
        out << "Largest File:  " << fs::path(maxFile).filename().string() << " (" << maxLines << " lines)\n";
        out << "              -> " << maxFile << "\n";
        out << "Smallest File: " << fs::path(minFile).filename().string() << " (" << minLines << " lines)\n";
        out << "              -> " << minFile << "\n";
        out << "================================================\n";
    }
}

// find the next available report filename
std::string getNextReportFilename(const fs::path& targetDir)
{
    std::string baseName = "cofee_report";
    std::string extension = ".txt";

    // check if base file exists
    fs::path reportPath = targetDir / (baseName + extension);
    if (!fs::exists(reportPath))
    {
        return reportPath.string();
    }

    // find next available number
    int counter = 1;
    while (true)
    {
        std::string numberedName = baseName + "[" + std::to_string(counter) + "]" + extension;
        reportPath = targetDir / numberedName;

        if (!fs::exists(reportPath))
        {
            return reportPath.string();
        }
        counter++;
    }
}

int main(int argc, char* argv[])
{
    std::string path = ".";
    bool generateReport = false;
    bool verbose = false;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--report" || arg == "-r")
        {
            generateReport = true;
        }
        else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "COFEE - Code Counter Tool\n";
            std::cout << "Usage: cofee [path] [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -r, --report    Generate a report file\n";
            std::cout << "  -v, --verbose   Show detailed analytics\n";
            std::cout << "  -h, --help      Show this help message\n\n";
            std::cout << "Examples:\n";
            std::cout << "  cofee .                    # Scan current directory\n";
            std::cout << "  cofee C:\\MyProject -r     # Scan and generate report\n";
            std::cout << "  cofee . -r -v              # Scan with verbose report\n";
            return 0;
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

    std::string longestFile = "";
    int maxLines = -1;

    std::string shortestFile = "";
    int minLines = std::numeric_limits<int>::max();

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
            pathStr.find("build") != std::string::npos) ||
            pathStr.find("vendor") != std::string::npos ||      
            pathStr.find("packages") != std::string::npos ||    
            pathStr.find("lib") != std::string::npos ||         
            pathStr.find("target") != std::string::npos ||      
            pathStr.find("__pycache__") != std::string::npos || 
            pathStr.find(".next") != std::string::npos ||      
            pathStr.find(".nuxt") != std::string::npos
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

            if (fileRealLines > maxLines) {
                maxLines = fileRealLines;
                longestFile = pathStr;
            }

            if (fileRealLines < minLines) {
                minLines = fileRealLines;
                shortestFile = pathStr;
            }

            // progress indicator
            std::cout << "\r[Scanning] Files: " << fileCount
                << " | Lines: " << totalLines << "   " << std::flush;
        }
    }

    std::cout << "\r[Done] Scanned " << fileCount << " files.                                  \n";

    printReport(std::cout, path, statsMap, totalLines, verbose, longestFile, maxLines, shortestFile, minLines);

    if (generateReport)
    {
        // get the scanned project directory (where reports should be saved)
        fs::path targetDir = fs::absolute(path);
        std::string reportPath = getNextReportFilename(targetDir);

        std::ofstream reportFile(reportPath);
        if (reportFile.is_open())
        {
            printReport(reportFile, path, statsMap, totalLines, verbose, longestFile, maxLines, shortestFile, minLines);
            reportFile.close();
            std::cout << "\n[Success] Report saved to:\n";
            std::cout << "  -> " << reportPath << "\n";
        }
        else
        {
            std::cerr << "[Error] Could not write report file.\n";
        }
    }

    return 0;
}