# Student Data Extractor

A local Windows CLI application for searching academic Excel files by matriculation number and exporting consolidated results to a new workbook.

## Features

- Browse directories interactively from the terminal
- Search a chosen folder or the full `RESULTS` tree recursively
- Discover `.xlsx` workbooks and process them safely
- Detect matriculation-number columns using header aliases
- Extract each matching row without discarding duplicates
- Combine records into a consolidated workbook in `OUTPUTS`
- Continue processing even if some files are unreadable
- Log session details for debugging

## Requirements

- Windows 10/11
- CMake 3.16+
- C++20 compatible compiler (MSVC or MinGW/GCC)
- OpenXLSX library for reading/writing Excel files

## Building

From PowerShell:

```powershell
cd C:\path\to\StudentDataExtractor
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Or with Visual Studio generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Running

```powershell
cd C:\path\to\StudentDataExtractor
.\build\StudentDataExtractor.exe
```

## Project structure

```text
StudentDataExtractor/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── app/
│   ├── filesystem/
│   ├── excel/
│   ├── extraction/
│   ├── models/
│   ├── ui/
│   ├── utils/
│   └── main.cpp
├── tests/
├── RESULTS/
├── OUTPUTS/
├── logs/
└── build/
```

## How to use

1. Place all source Excel files under `RESULTS/` in any nested folder structure.
2. Run the app from PowerShell.
3. Select a folder to browse or search the entire `RESULTS` tree.
4. Enter the student's matriculation number.
5. Wait for the extraction to complete.
6. The consolidated workbook is written to `OUTPUTS/`.

## Notes

- The generated workbook is always written under an application-managed `OUTPUTS` directory.
- The app ignores output files during input traversal.
- It treats empty workbook cells as empty values rather than `0`.
- Duplicate matching rows are preserved as separate output rows with a warning.

## Limitations

- Version 1 focuses on `.xlsx` files.
- Complex column normalization is intentionally minimal.
- The app supports local file processing only.

## Future improvements

- GUI front end
- CSV support
- Better header normalization
- CSV export option
- Search by name or course
- More reporting and statistics
