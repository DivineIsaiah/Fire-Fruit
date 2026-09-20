# Student Data Extractor

A Windows desktop application for searching academic Excel files by matriculation number and exporting consolidated results to a new workbook. A command-line interface is also available for development and scripted use.

## Features

- Browse directories interactively from the terminal
- Search a chosen folder or the full `RESULTS` tree recursively
- Discover `.xlsx` workbooks and process them safely
- Detect matriculation-number columns using header aliases
- Extract each matching row without discarding duplicates
- Combine records into a consolidated workbook in `OUTPUTS`
- Continue processing even if some files are unreadable
- Log session details for debugging
- Native Windows desktop GUI
- Remembers the selected Results folder between launches
- Includes a self-contained Windows installer

## Installing For Windows Users

Download and run:

```text
StudentDataExtractor-Setup.exe
```

The installer includes the application runtime and creates a Start Menu shortcut for **Student Data Extractor**. No Git, CMake, MinGW, Visual Studio, OpenXLSX, or other development tools are required.

After installation:

1. Open **Student Data Extractor** from the Start Menu.
2. Select the folder containing the academic result workbooks.
3. Select an academic session.
4. Enter the student's matriculation number.
5. Click **Search**.
6. Click **Open Result** when extraction completes.

The application displays progress, matching summaries, processing errors, and no-match results in the window. Generated workbooks are saved in the user's application data folder and can be opened directly with **Open Result**.

## Developer Requirements

- Windows 10/11
- CMake 3.16+
- C++20 compatible compiler (MSVC or MinGW/GCC)
- OpenXLSX library for reading/writing Excel files

## Building From Source

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

## Running The CLI

```powershell
cd C:\path\to\StudentDataExtractor
.\build\StudentDataExtractor.exe
```

To launch the GUI from a source build:

```powershell
.\build\StudentDataExtractorGui.exe
```

The GUI is the recommended interface for normal Windows users. The CLI remains available for development and troubleshooting.

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
│   ├── gui/
│   ├── utils/
│   └── main.cpp
├── tests/
├── RESULTS/
├── OUTPUTS/
├── logs/
└── build/
```

## CLI Usage

1. Place all source Excel files under `RESULTS/` in any nested folder structure.
2. Run the app from PowerShell.
3. Select a folder to browse or search the entire `RESULTS` tree.
4. Enter the student's matriculation number.
5. Wait for the extraction to complete.
6. The consolidated workbook is written to `OUTPUTS/`.

## GUI Usage

The GUI discovers academic-session folders directly inside the selected Results folder. It validates the folder, session, and matriculation number before enabling **Search**. Extraction runs in the background so the window remains responsive.

The GUI reports:

- Files checked
- Matching files
- Records extracted
- Files with no match
- Files with errors
- The generated result filename

The selected Results folder is stored in the user's local Windows application data and restored on the next launch if it still exists.

## Notes

- The CLI writes generated workbooks under the application-managed `OUTPUTS` directory. Installed GUI builds use the user's local application data directory.
- The app ignores output files during input traversal.
- It treats empty workbook cells as empty values rather than `0`.
- Duplicate matching rows are preserved as separate output rows with a warning.

## Limitations

- The application focuses on `.xlsx` files.
- Complex column normalization is intentionally minimal.
- The app supports local file processing only.

## Future Improvements

- CSV support
- Better header normalization
- CSV export option
- Search by name or course
- More reporting and statistics
