#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include <iomanip>
#include <limits>
#include <thread> 
#include <mutex>  
#include <atomic> 
#include <chrono>

namespace fs = std::filesystem;

// --- shared data structures ---

struct LanguageStats {
    int fileCount = 0;
    int codeLines = 0;
};

std::mutex g_statsMutex;


bool isCodeFile(const fs::path& filePath) {
    std::string ext = filePath.extension().string();
    return (ext == ".cpp" || ext == ".h" || ext == ".hpp" ||
        ext == ".c" || ext == ".cs" ||
        ext == ".js" || ext == ".ts" ||
        ext == ".jsx" || ext == ".tsx" ||
        ext == ".css" || ext == ".scss" || ext == ".html" ||
        ext == ".vue" || ext == ".json");
}

bool hasRealCode(const std::string& line, bool& inBlockComment) {
    bool foundCode = false;
    for (size_t i = 0; i < line.length(); i++) {
        if (inBlockComment) {
            if (i + 1 < line.length() && line[i] == '*' && line[i + 1] == '/') {
                inBlockComment = false; i++;
            }
            continue;
        }
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '*') {
            inBlockComment = true; i++; continue;
        }
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '/') {
            break;
        }
        if (!std::isspace(static_cast<unsigned char>(line[i]))) {
            foundCode = true;
        }
    }
    return foundCode;
}


void printReport(std::ostream& out, const std::string& path,
    const std::map<std::string, LanguageStats>& statsMap,
    int totalLines,
    bool verbose,
    const std::string& maxFile, int maxLines,
    const std::string& minFile, int minLines)
{
    out << "------------------------------------------------\n";
    out << "PROJECT SCAN REPORT: " << path << "\n";
    out << "------------------------------------------------\n";
    out << std::left << std::setw(15) << "TYPE"
        << std::setw(15) << "FILES"
        << std::setw(15) << "LINES (CODE)" << "\n";
    out << "------------------------------------------------\n";

    for (const auto& [ext, stat] : statsMap) {
        out << std::left << std::setw(15) << ext
            << std::setw(15) << stat.fileCount
            << std::setw(15) << stat.codeLines << "\n";
    }
    out << "------------------------------------------------\n";
    out << "TOTAL REAL CODE: " << totalLines << "\n";
    out << "------------------------------------------------\n";

    if (verbose) {
        out << "\n[VERBOSE ANALYTICS]\n";
        out << "Largest File:  " << fs::path(maxFile).filename().string() << " (" << maxLines << " lines)\n";
        out << "              -> " << maxFile << "\n";
        out << "Smallest File: " << fs::path(minFile).filename().string() << " (" << minLines << " lines)\n";
        out << "              -> " << minFile << "\n";
        out << "------------------------------------------------\n";
    }
}


void workerFunction(const std::vector<fs::path>& filesToProcess,
    std::map<std::string, LanguageStats>& statsMap,
    std::atomic<int>& totalCodeLines,
    std::atomic<int>& processedFileCount,
    std::string& currentLongestFile, std::atomic<int>& currentMaxLines,
    std::string& currentShortestFile, std::atomic<int>& currentMinLines)
{
    bool inBlockComment = false; //local state for block comments

    for (const auto& filePath : filesToProcess)
    {
        std::ifstream fileReader(filePath.string());
        if (!fileReader.is_open()) continue;

        std::string ext = filePath.extension().string();
        std::string line;
        int fileRealLines = 0;

        inBlockComment = false;
        while (std::getline(fileReader, line)) {
            if (hasRealCode(line, inBlockComment)) {
                fileRealLines++;
            }
        }

        // std::lock_guard automatically locks the mutex and unlocks it when it goes out of scope.
        std::lock_guard<std::mutex> lock(g_statsMutex);

        statsMap[ext].fileCount++;
        statsMap[ext].codeLines += fileRealLines;

        totalCodeLines += fileRealLines;
        processedFileCount++;

        // update longest/shortest
        if (fileRealLines > currentMaxLines) {
            currentMaxLines = fileRealLines;
            currentLongestFile = filePath.string(); // this string assignment is thread safe for unique strings
        }
        if (fileRealLines < currentMinLines) {
            currentMinLines = fileRealLines;
            currentShortestFile = filePath.string();
        }
    }
}

void printHelp() {
    std::cout << "================================================\n";
    std::cout << " COFEE - Codebase Frequency & Efficiency Engine \n";
    std::cout << "================================================\n";
    std::cout << "Usage:\n";
    std::cout << "  cofee <path> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -v, --verbose    Show largest and smallest files.\n";
    std::cout << "  -r, --report     Save a summary to 'cofee_report.txt'.\n";
    std::cout << "  -h, --help       Show this help message.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  cofee .\n";
    std::cout << "  cofee C:\\MyProject -v\n";
    std::cout << "  cofee E:\\V33 -r -v\n";
    std::cout << "================================================\n";
}

// --- main functionality ---

int main(int argc, char* argv[])
{
    auto start_time = std::chrono::high_resolution_clock::now();

    std::string path = ".";
    bool generateReport = false;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--report" || arg == "-r") {
            generateReport = true;
        }
        else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
        else if (arg == "--help" || arg == "-h") { 
            printHelp();
            return 0; 
        }

        else if (arg[0] != '-') {
            path = arg;
        }
    }

    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cerr << "Error: The path '" << path << "' does not exist or is not a directory.\n";
        return 1;
    }

    // --- data to be shared and updated by threads ---
    std::map<std::string, LanguageStats> statsMap;
    std::atomic<int> totalCodeLines = 0; 
    std::atomic<int> processedFileCount = 0; 

    std::string longestFile = "";
    std::atomic<int> maxLines = -1;

    std::string shortestFile = "";
    std::atomic<int> minLines = std::numeric_limits<int>::max();

    // collect all files to be processed
    std::vector<fs::path> allFiles;
    std::cout << "Collecting files in " << path << "...\n";
    for (auto const& entry : fs::recursive_directory_iterator(path))
    {
        std::string pathStr = entry.path().string();

        if (pathStr.find("node_modules") != std::string::npos ||
            pathStr.find(".git") != std::string::npos ||
            pathStr.find("dist") != std::string::npos ||
            pathStr.find(".vs") != std::string::npos ||
            pathStr.find("vendor") != std::string::npos ||      
            pathStr.find("packages") != std::string::npos ||    
            pathStr.find("lib") != std::string::npos ||         
            pathStr.find("target") != std::string::npos ||      
            pathStr.find("__pycache__") != std::string::npos || 
            pathStr.find(".next") != std::string::npos || 
            pathStr.find(".nuxt") != std::string::npos ||
            pathStr.find("build") != std::string::npos) {
            continue;
        }

        if (entry.is_regular_file() && isCodeFile(entry.path())) {
            allFiles.push_back(entry.path());
        }
    }
    std::cout << "Found " << allFiles.size() << " relevant files. Starting scan...\n";

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1; 

    std::vector<std::thread> threads;
    std::vector<std::vector<fs::path>> threadFileChunks(numThreads);

    // distribute files among threads
    for (size_t i = 0; i < allFiles.size(); ++i) {
        threadFileChunks[i % numThreads].push_back(allFiles[i]);
    }

    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerFunction, std::cref(threadFileChunks[i]),
            std::ref(statsMap),
            std::ref(totalCodeLines),
            std::ref(processedFileCount),
            std::ref(longestFile), std::ref(maxLines),
            std::ref(shortestFile), std::ref(minLines));
    }

    while (processedFileCount < allFiles.size()) {
        std::cout << "\r[Scanning] Files: " << processedFileCount
            << "/" << allFiles.size() << " | Lines: " << totalCodeLines << "   " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "\r[Done] Scanned " << processedFileCount << "/" << allFiles.size() << " files.                                  \n";

    if (allFiles.empty()) minLines = 0;

    // print reports
    printReport(std::cout, path, statsMap, totalCodeLines, verbose, longestFile, maxLines, shortestFile, minLines);

    if (generateReport) {
        std::ofstream reportFile("cofee_report.txt");
        if (reportFile.is_open()) {
            printReport(reportFile, path, statsMap, totalCodeLines, verbose, longestFile, maxLines, shortestFile, minLines);
            std::cout << "[Success] Report saved to 'cofee_report.txt'\n";
        }
        else {
            std::cerr << "[Error] Could not write report file.\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "Execution time: " << std::fixed << std::setprecision(3) << duration.count() << " seconds\n"; 

    return 0;
}