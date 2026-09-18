#pragma once

#include <map>
#include <string>
#include <vector>

namespace sde {

struct StudentRecord {
    std::string subject;
    std::string sourceFile;
    std::map<std::string, std::string> fields;
    bool matched = false;
};

struct ExtractionSummary {
    int filesDiscovered = 0;
    int excelFilesProcessed = 0;
    int studentsFound = 0;
    int filesWithNoMatch = 0;
    int filesWithErrors = 0;
    int recordsExtracted = 0;
    std::string outputPath;
};

struct FileError {
    std::string filePath;
    std::string reason;
};

struct ExtractionNotification {
    std::vector<StudentRecord> records;
    std::vector<FileError> errors;
    std::vector<std::string> warnings;
    int processed = 0;
    int matches = 0;
    int noMatch = 0;
    std::string outputPath;
};

} // namespace sde
