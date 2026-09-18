#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "models/ApplicationConfig.h"
#include "models/StudentRecord.h"

namespace sde {

struct WorkbookExtractionResult {
    std::vector<StudentRecord> records;
    std::vector<FileError> errors;
    std::vector<std::string> warnings;
    int matches = 0;
    int noMatch = 0;
};

class StudentExtractor {
public:
    StudentExtractor(ApplicationConfig config, std::string targetMatricNumber);
    WorkbookExtractionResult processWorkbook(const std::filesystem::path& workbookPath) const;

private:
    ApplicationConfig config_;
    std::string targetMatricNumber_;
};

} // namespace sde
