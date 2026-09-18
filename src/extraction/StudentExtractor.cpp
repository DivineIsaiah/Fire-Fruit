#include "extraction/StudentExtractor.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include <OpenXLSX.hpp>

#include "extraction/ColumnMatcher.h"
#include "utils/StringUtils.h"

namespace sde {

namespace {

std::string cellValueToString(const OpenXLSX::XLCell& cell) {
    try {
        const OpenXLSX::XLCellValue value = cell.value();
        switch (value.type()) {
            case OpenXLSX::XLValueType::Empty:
                return "";
            case OpenXLSX::XLValueType::Boolean:
                return value.get<bool>() ? "TRUE" : "FALSE";
            case OpenXLSX::XLValueType::Integer:
                return std::to_string(value.get<int64_t>());
            case OpenXLSX::XLValueType::Float:
                return std::to_string(value.get<double>());
            case OpenXLSX::XLValueType::String:
                return value.get<const char*>();
            case OpenXLSX::XLValueType::Error:
                return "";
            default:
                return "";
        }
    } catch (const std::exception&) {
        return "";
    }
}

std::vector<std::string> rowValues(const OpenXLSX::XLWorksheet& sheet, uint32_t rowNumber, uint16_t maxColumns) {
    std::vector<std::string> values;
    values.reserve(maxColumns);
    for (uint16_t col = 1; col <= maxColumns; ++col) {
        values.push_back(cellValueToString(sheet.cell(rowNumber, col)));
    }
    return values;
}

} // namespace

StudentExtractor::StudentExtractor(ApplicationConfig config, std::string targetMatricNumber)
    : config_(std::move(config)), targetMatricNumber_(trim(std::move(targetMatricNumber))) {}

WorkbookExtractionResult StudentExtractor::processWorkbook(const std::filesystem::path& workbookPath) const {
    WorkbookExtractionResult result;

    try {
        if (!std::filesystem::exists(workbookPath)) {
            throw std::runtime_error("Workbook does not exist");
        }

        auto ext = workbookPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        });
        if (ext != ".XLSX" && ext != ".XLS") {
            throw std::runtime_error("Unsupported workbook extension");
        }

        OpenXLSX::XLDocument doc;
        doc.open(workbookPath.string());

        auto workbook = doc.workbook();
        const auto sheetNames = workbook.worksheetNames();
        if (sheetNames.empty()) {
            throw std::runtime_error("No worksheets were found in the workbook");
        }

        const std::string subject = workbookPath.stem().string();
        const std::vector<std::string> aliases = config_.matricHeaderAliases;

        for (const auto& sheetName : sheetNames) {
            const auto sheet = workbook.worksheet(sheetName);
            if (sheet.rowCount() == 0) {
                continue;
            }

            const auto maxColumns = std::max<uint16_t>(sheet.columnCount(), 1);
            std::map<int, std::string> headerMap;
            std::vector<std::string> headerRow;
            bool headerDetected = false;
            int matricColumn = -1;

            for (uint32_t row = 1; row <= sheet.rowCount(); ++row) {
                auto rowValuesVector = rowValues(sheet, row, maxColumns);
                if (row == 1) {
                    headerRow = rowValuesVector;
                }

                for (uint16_t col = 1; col <= maxColumns; ++col) {
                    const auto headerName = (row == 1) ? rowValuesVector[col - 1] : "";
                    if (row == 1 && ColumnMatcher::isMatricColumnName(headerName, aliases)) {
                        matricColumn = static_cast<int>(col);
                        headerDetected = true;
                        break;
                    }
                }

                if (headerDetected) {
                    break;
                }
            }

            for (uint32_t row = 1; row <= sheet.rowCount(); ++row) {
                const auto currentRow = rowValues(sheet, row, maxColumns);
                if (currentRow.empty()) {
                    continue;
                }

                if (row == 1) {
                    std::vector<std::string> headerNames = currentRow;
                    for (uint16_t col = 1; col <= maxColumns; ++col) {
                        const auto name = col <= headerNames.size() ? trim(headerNames[col - 1]) : "";
                        if (!name.empty()) {
                            headerMap[static_cast<int>(col)] = name;
                        }
                    }
                    continue;
                }

                if (matricColumn == -1) {
                    for (uint16_t col = 1; col <= maxColumns; ++col) {
                        const auto value = col <= currentRow.size() ? trim(currentRow[col - 1]) : "";
                        if (ColumnMatcher::matchesMatricValue(value, targetMatricNumber_)) {
                            matricColumn = static_cast<int>(col);
                            break;
                        }
                    }
                }

                if (matricColumn == -1) {
                    continue;
                }

                const auto matricValue = matricColumn > 0 && matricColumn <= static_cast<int>(currentRow.size())
                    ? trim(currentRow[matricColumn - 1])
                    : "";
                if (!ColumnMatcher::matchesMatricValue(matricValue, targetMatricNumber_)) {
                    continue;
                }

                StudentRecord record;
                record.subject = subject;
                record.sourceFile = workbookPath.string();
                record.matched = true;

                for (uint16_t col = 1; col <= maxColumns; ++col) {
                    const auto key = (col <= currentRow.size()) ? trim(currentRow[col - 1]) : "";
                    const auto columnName = headerMap.count(static_cast<int>(col)) ? headerMap[static_cast<int>(col)] : "Column " + std::to_string(col);
                    const auto normalizedValue = (col <= currentRow.size()) ? currentRow[col - 1] : "";
                    if (!columnName.empty() && !normalizedValue.empty()) {
                        record.fields[columnName] = normalizedValue;
                    }
                }

                if (record.fields.empty()) {
                    record.fields["Matric Number"] = matricValue;
                }

                record.fields["Subject"] = subject;
                record.fields["Matric Number"] = matricValue;
                result.records.push_back(record);
                ++result.matches;
            }
        }

        if (result.records.empty()) {
            result.noMatch = 1;
            result.warnings.push_back("No matching rows found in workbook: " + workbookPath.filename().string());
        }

        doc.close();
    } catch (const std::exception& ex) {
        result.errors.push_back({workbookPath.string(), ex.what()});
    }

    return result;
}

} // namespace sde
