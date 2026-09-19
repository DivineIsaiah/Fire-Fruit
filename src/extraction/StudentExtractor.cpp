#include "extraction/StudentExtractor.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include <OpenXLSX.hpp>

#include "extraction/ColumnMatcher.h"
#include "utils/StringUtils.h"

namespace sde {

namespace {

std::string makeUniqueFieldName(const std::string& rawHeader, std::set<std::string>& usedNames, uint16_t columnIndex) {
    std::string normalized = trim(rawHeader);
    if (normalized.empty()) {
        normalized = "Column " + std::to_string(columnIndex);
    }

    std::string candidate = normalized;
    std::string sourceBase = normalized;
    const bool reservedName = equalsIgnoreCase(normalized, "Subject") || equalsIgnoreCase(normalized, "Matric Number");
    if (reservedName) {
        sourceBase = normalized + " (source)";
        candidate = sourceBase;
    }

    int suffix = 1;
    while (usedNames.count(candidate) != 0 || equalsIgnoreCase(candidate, "Subject") || equalsIgnoreCase(candidate, "Matric Number")) {
        std::ostringstream suffixStream;
        if (reservedName) {
            suffixStream << normalized << " (source " << (suffix + 1) << ")";
        } else {
            suffixStream << normalized << " (" << (suffix + 1) << ")";
        }
        candidate = suffixStream.str();
        ++suffix;
    }

    usedNames.insert(candidate);
    return candidate;
}

std::string cellValueToString(const OpenXLSX::XLCell& cell) {
    try {
        const auto& value = cell.value();
        switch (value.type()) {
            case OpenXLSX::XLValueType::Empty:
                return "";
            case OpenXLSX::XLValueType::Boolean:
                return value.get<bool>() ? "TRUE" : "FALSE";
            case OpenXLSX::XLValueType::Integer:
                return std::to_string(value.get<int64_t>());
            case OpenXLSX::XLValueType::Float: {
                std::ostringstream stream;
                stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value.get<double>();
                return stream.str();
            }
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
            std::vector<std::string> headerRowValues;
            int headerRowIndex = -1;
            int matricColumn = -1;

            for (uint32_t row = 1; row <= sheet.rowCount(); ++row) {
                const auto rowValuesVector = rowValues(sheet, row, maxColumns);
                for (uint16_t col = 1; col <= maxColumns; ++col) {
                    const auto headerName = col <= rowValuesVector.size() ? trim(rowValuesVector[col - 1]) : "";
                    if (ColumnMatcher::isMatricColumnName(headerName, aliases)) {
                        headerRowIndex = static_cast<int>(row);
                        matricColumn = static_cast<int>(col);
                        headerRowValues = rowValuesVector;
                        break;
                    }
                }
                if (headerRowIndex != -1) {
                    break;
                }
            }

            if (headerRowIndex == -1) {
                continue;
            }

            for (uint32_t row = static_cast<uint32_t>(headerRowIndex + 1); row <= sheet.rowCount(); ++row) {
                const auto currentRow = rowValues(sheet, row, maxColumns);
                if (currentRow.empty()) {
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
                std::set<std::string> usedFieldNames;

                for (uint16_t col = 1; col <= maxColumns; ++col) {
                    const auto columnName = col <= headerRowValues.size() ? trim(headerRowValues[col - 1]) : "";
                    const auto normalizedValue = (col <= currentRow.size()) ? trim(currentRow[col - 1]) : "";
                    if (normalizedValue.empty()) {
                        continue;
                    }

                    std::string fieldName = columnName;
                    if (fieldName.empty()) {
                        fieldName = "Column " + std::to_string(col);
                    }

                    if (ColumnMatcher::isMatricColumnName(fieldName, aliases)) {
                        continue;
                    }
                    if (equalsIgnoreCase(fieldName, "S/NO") || equalsIgnoreCase(fieldName, "SN") || equalsIgnoreCase(fieldName, "SERIAL NO")) {
                        continue;
                    }

                    const auto uniqueKey = makeUniqueFieldName(fieldName, usedFieldNames, col);
                    record.fields[uniqueKey] = normalizedValue;
                }

                if (record.fields.count("Subject") == 0) {
                    record.fields["Subject"] = subject;
                }
                if (record.fields.count("Matric Number") == 0) {
                    record.fields["Matric Number"] = matricValue;
                }
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
