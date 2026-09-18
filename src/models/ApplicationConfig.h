#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sde {

struct ApplicationConfig {
    std::filesystem::path appRoot;
    std::filesystem::path resultsRoot;
    std::filesystem::path outputsRoot;
    std::filesystem::path logsRoot;
    std::vector<std::string> supportedExtensions{".xlsx", ".xls"};
    std::vector<std::string> matricHeaderAliases = {
        "matric number", "matric no", "matric no.", "matriculation number",
        "matriculation no", "registration number", "reg no", "reg. no.", "student id",
        "studentid", "matriculation", "student number"
    };
};

} // namespace sde
