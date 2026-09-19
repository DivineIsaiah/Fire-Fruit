#include <filesystem>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <OpenXLSX.hpp>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: workbook_inspector <workbook.xlsx>\n";
        return 1;
    }

    std::filesystem::path workbookPath = argv[1];

    if (!std::filesystem::exists(workbookPath)) {
        std::cerr << "Workbook not found: " << workbookPath << '\n';
        return 1;
    }

    OpenXLSX::XLDocument doc;
    doc.open(workbookPath.string());
    auto workbook = doc.workbook();

    std::cout << "Workbook: " << workbookPath << '\n';
    for (const auto& sheetName : workbook.worksheetNames()) {
        auto sheet = workbook.worksheet(sheetName);
        std::cout << "Sheet: " << sheetName << "\n";
        std::cout << "Rows: " << sheet.rowCount() << ", Columns: " << sheet.columnCount() << "\n";
        for (uint32_t row = 1; row <= std::min<uint32_t>(sheet.rowCount(), 12); ++row) {
            std::cout << "Row " << row << ": ";
            for (uint16_t col = 1; col <= std::min<uint16_t>(sheet.columnCount(), 20); ++col) {
                const auto& value = sheet.cell(row, col).value();
                std::string text;
                switch (value.type()) {
                    case OpenXLSX::XLValueType::Empty:
                        text = "<empty>";
                        break;
                    case OpenXLSX::XLValueType::Boolean:
                        text = value.get<bool>() ? "TRUE" : "FALSE";
                        break;
                    case OpenXLSX::XLValueType::Integer:
                        text = std::to_string(value.get<int64_t>());
                        break;
                    case OpenXLSX::XLValueType::Float:
                        text = std::to_string(value.get<double>());
                        break;
                    case OpenXLSX::XLValueType::String:
                        text = value.get<const char*>();
                        break;
                    default:
                        text = "<other>";
                        break;
                }
                std::cout << "[" << col << "]" << text << " | ";
            }
            std::cout << '\n';
        }
    }

    doc.close();
    return 0;
}
