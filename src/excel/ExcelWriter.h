#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "models/StudentRecord.h"

namespace sde {

class ExcelWriter {
public:
    explicit ExcelWriter(std::filesystem::path outputDirectory);
    std::filesystem::path writeWorkbook(const std::vector<StudentRecord>& records, const std::string& matricNumber) const;

private:
    std::filesystem::path outputDirectory_;
};

} // namespace sde
