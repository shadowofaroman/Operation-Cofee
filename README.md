# COFEE

**Codebase Frequency & Efficiency Engine**

A high-performance C++ command-line tool for analyzing source code metrics across large-scale projects. COFEE provides accurate line counts by intelligently filtering comments, whitespace, and dependency directories while leveraging multi-threaded architecture for optimal performance.

## Overview

COFEE addresses a fundamental challenge in software engineering: accurately measuring codebase size while excluding noise from dependencies, comments, and generated files. Unlike traditional line counters, COFEE combines intelligent parsing with parallel processing to deliver fast, accurate results even on enterprise-scale codebases.

### Key Capabilities

- **Intelligent Code Analysis**: Distinguishes actual code from comments (`//`, `/* */`) and whitespace
- **Dependency-Aware Scanning**: Automatically excludes common dependency directories (`node_modules`, `vendor`, `.git`, `build`, `dist`, `packages`, `target`, `__pycache__`, `.next`, `.nuxt`)
- **Multi-Language Support**: Handles C/C++, C#, JavaScript/TypeScript, JSX/TSX, CSS/SCSS, HTML, Vue, and JSON
- **Parallel Processing**: Leverages multi-threading across available CPU cores for efficient file processing
- **Detailed Analytics**: Per-language breakdowns with optional verbose statistics

## Installation

### Prerequisites

- Windows operating system (Windows 10 or later recommended)
- Visual Studio 2022 with C++ development tools (for building from source)

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/shadowofaroman/Operation-Cofee.git
cd Operation-Cofee
```

2. Open `cofee.sln` in Visual Studio 2022

3. Configure build settings:
   - Configuration: **Release**
   - Platform: **x64**
   - C++ Language Standard: **ISO C++17** or later

4. Build the solution (`Ctrl+Shift+B`)

5. The compiled executable will be located at: `x64/Release/cofee.exe`

### System Path Configuration (Optional)

To run COFEE from any directory:

1. Copy `cofee.exe` to a permanent location (e.g., `C:\Tools\cofee\`)
2. Add this location to your system PATH:
   - Open System Properties → Environment Variables
   - Edit the `Path` variable under System variables
   - Add your COFEE directory path
   - Click OK to save changes

## Usage

### Basic Syntax
```bash
cofee <path> [options]
```

### Options

| Flag | Description |
|------|-------------|
| `-v, --verbose` | Display detailed analytics including largest and smallest files |
| `-r, --report` | Generate a summary report saved to `cofee_report.txt` |
| `-h, --help` | Display help information |

### Examples

**Scan current directory:**
```bash
cofee .
```

**Scan specific project with verbose output:**
```bash
cofee C:\Projects\MyApplication -v
```

**Generate report for project:**
```bash
cofee E:\Development\WebApp -r -v
```

**Scan and save report:**
```bash
cofee . -r
```

## Output Format

### Standard Output
```
------------------------------------------------
PROJECT SCAN REPORT: C:\Projects\MyApp
------------------------------------------------
TYPE            FILES          LINES (CODE)   
------------------------------------------------
.cpp            45             12,847         
.h              38             8,293          
.js             124            31,456         
.css            18             4,102          
------------------------------------------------
TOTAL REAL CODE: 56,698
------------------------------------------------
```

### Verbose Output

When using the `-v` flag, additional metrics are displayed:
```
[VERBOSE ANALYTICS]
Largest File:  MainController.cpp (1,247 lines)
              -> C:\Projects\MyApp\src\MainController.cpp
Smallest File: Config.h (12 lines)
              -> C:\Projects\MyApp\include\Config.h
------------------------------------------------
Execution time: 0.847 seconds
```

## Architecture

### Multi-Threading Implementation

COFEE distributes file processing across worker threads equal to the number of available CPU cores. Each thread:

1. Maintains local state for block comment tracking
2. Processes an assigned subset of files
3. Uses mutex-protected updates for shared statistics
4. Employs atomic operations for counters to prevent race conditions

### Code Detection Algorithm

The parser implements a state machine that:

- Tracks multi-line comment blocks (`/* ... */`)
- Identifies and skips single-line comments (`//`)
- Filters whitespace-only lines
- Preserves context across line boundaries

### Performance Characteristics

| Project Scale | Files | Typical Execution Time |
|--------------|-------|------------------------|
| Small (~10K LOC) | ~50 | < 0.1 seconds |
| Medium (~50K LOC) | ~200 | ~0.3 seconds |
| Large (~200K LOC) | ~500 | ~1.0 seconds |

*Performance measured on Intel Core i7 with 8 threads*

## Supported File Types

- **C/C++**: `.c`, `.cpp`, `.h`, `.hpp`
- **C#**: `.cs`
- **JavaScript/TypeScript**: `.js`, `.ts`, `.jsx`, `.tsx`
- **Styling**: `.css`, `.scss`
- **Markup**: `.html`, `.vue`
- **Data**: `.json`

## Known Limitations

1. **Platform**: Currently Windows-only due to filesystem API usage
2. **Report Versioning**: Multiple report generations overwrite `cofee_report.txt` (no automatic versioning implemented yet)
3. **Language Detection**: Based solely on file extensions
4. **Comment Handling**: Does not recognize language-specific comment syntax (e.g., Python `#`, SQL `--`)

## Contributing

Contributions are welcome. Please ensure:

- Code follows existing style conventions
- New features include appropriate error handling
- Thread safety is maintained for shared data structures
- Changes are tested on projects of varying sizes

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Author

**A mouse** ([@TheIndieRoman](https://x.com/TheIndieRoman))

---

*Built with C++17 | Powered by std::filesystem and std::thread*