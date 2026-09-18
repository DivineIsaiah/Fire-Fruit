#include "excel/ExcelWriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

#include <OpenXLSX.hpp>

#include "utils/StringUtils.h"

namespace sde {

ExcelWriter::ExcelWriter(std::filesystem::path outputDirectory)
    : outputDirectory_(std::filesystem::absolute(outputDirectory)) {
    std::filesystem::create_directories(outputDirectory_);
}

std::filesystem::path ExcelWriter::writeWorkbook(const std::vector<StudentRecord>& records, const std::string& matricNumber) const {
    std::filesystem::create_directories(outputDirectory_);
    auto safeMatric = sanitizeFilename(matricNumber);
    std::replace(safeMatric.begin(), safeMatric.end(), '/', '_');
    std::replace(safeMatric.begin(), safeMatric.end(), '\\', '_');
    std::replace(safeMatric.begin(), safeMatric.end(), ':', '_');

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream filename;
    filename << "extraction_" << (safeMatric.empty() ? "student" : safeMatric) << "_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".xlsx";
    std::filesystem::path outputPath = outputDirectory_ / filename.str();

    OpenXLSX::XLDocument doc;
    doc.create(outputPath.string());
    auto sheet = doc.workbook().worksheet("Sheet1");

    std::vector<std::string> columns = {"Subject", "Matric Number"};
    std::set<std::string> seen;
    seen.insert("Subject");
    seen.insert("Matric Number");

    for (const auto& record : records) {
        for (const auto& [key, value] : record.fields) {
            if (key != "Subject" && key != "Matric Number" && !seen.count(key)) {
                columns.push_back(key);
                seen.insert(key);
            }
        }
    }

    for (size_t col = 0; col < columns.size(); ++col) {
        sheet.cell(1, static_cast<uint16_t>(col + 1)).value() = columns[col];
    }

    size_t rowIndex = 2;
    for (const auto& record : records) {
        std::map<std::string, std::string> rowMap;
        for (const auto& [key, value] : record.fields) {
            rowMap[key] = value;
        }
        for (size_t col = 0; col < columns.size(); ++col) {
            const auto& columnName = columns[col];
            auto it = rowMap.find(columnName);
            if (it != rowMap.end() && !it->second.empty()) {
                sheet.cell(static_cast<uint32_t>(rowIndex), static_cast<uint16_t>(col + 1)).value() = it->second;
            }
        }
        ++rowIndex;
    }

    doc.save();
    return outputPath;
}

} // namespace sde
