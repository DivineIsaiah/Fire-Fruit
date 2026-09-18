#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <OpenXLSX.hpp>

#include "extraction/StudentExtractor.h"
#include "filesystem/FileSearcher.h"
#include "utils/StringUtils.h"

namespace {

void createWorkbook(const std::filesystem::path& path, const std::vector<std::vector<std::string>>& rows) {
    OpenXLSX::XLDocument doc;
    doc.create(path.string());
    auto wks = doc.workbook().worksheet("Sheet1");

    for (size_t r = 0; r < rows.size(); ++r) {
        for (size_t c = 0; c < rows[r].size(); ++c) {
            wks.cell(static_cast<uint32_t>(r + 1), static_cast<uint16_t>(c + 1)).value() = rows[r][c];
        }
    }
    doc.save();
}

bool testStringUtils() {
    const std::string sample = "  MIVA/CSC/21/1234  \n";
    const auto trimmed = sde::trim(sample);
    if (trimmed != "MIVA/CSC/21/1234") {
        return false;
    }
    if (!sde::equalsIgnoreCase("Matric No", "matric no")) {
        return false;
    }
    if (sde::sanitizeFilename("A/B:C?\"<D>|E") != "A_B_C___D__E") {
        return false;
    }
    return true;
}

bool testFileSearcher() {
    auto tempRoot = std::filesystem::temp_directory_path() / "sde_file_search_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "RESULTS" / "2026");
    std::filesystem::create_directories(tempRoot / "OUTPUTS");
    createWorkbook(tempRoot / "RESULTS" / "2026" / "one.xlsx", {{"Header"}, {"value"}});
    createWorkbook(tempRoot / "RESULTS" / "2026" / "two.xls", {{"Header"}, {"value"}});
    createWorkbook(tempRoot / "OUTPUTS" / "generated.xlsx", {{"Header"}, {"value"}});

    sde::FileSearcher searcher(tempRoot / "RESULTS", tempRoot / "OUTPUTS");
    auto files = searcher.findExcelFiles();
    if (files.size() != 2) {
        return false;
    }

    std::filesystem::remove_all(tempRoot);
    return true;
}

bool testStudentExtractor() {
    auto tempRoot = std::filesystem::temp_directory_path() / "sde_extractor_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "RESULTS");
    std::filesystem::create_directories(tempRoot / "OUTPUTS");

    const std::vector<std::vector<std::string>> rows = {
        {"Matric No", "Name", "CA", "Exam", "Total", "Grade"},
        {"MIVA/CSC/21/1234", "Divine Isaiah", "28", "55", "83", "A"},
        {"MIVA/CSC/21/1234", "Divine Isaiah", "30", "60", "90", "A"},
        {"MIVA/CSC/21/9999", "Someone Else", "10", "20", "30", "C"}
    };
    const auto workbookPath = tempRoot / "RESULTS" / "CSC301.xlsx";
    createWorkbook(workbookPath, rows);

    sde::ApplicationConfig config;
    config.appRoot = tempRoot;
    config.resultsRoot = tempRoot / "RESULTS";
    config.outputsRoot = tempRoot / "OUTPUTS";

    sde::StudentExtractor extractor(config, "MIVA/CSC/21/1234");
    auto result = extractor.processWorkbook(workbookPath);

    if (result.records.size() != 2) {
        std::filesystem::remove_all(tempRoot);
        return false;
    }

    if (result.records[0].fields["Name"] != "Divine Isaiah") {
        std::filesystem::remove_all(tempRoot);
        return false;
    }

    if (result.records[1].fields["CA"] != "30") {
        std::filesystem::remove_all(tempRoot);
        return false;
    }

    std::filesystem::remove_all(tempRoot);
    return true;
}

} // namespace

int main() {
    const bool stringOk = testStringUtils();
    const bool fileOk = testFileSearcher();
    const bool extractorOk = testStudentExtractor();

    if (!stringOk || !fileOk || !extractorOk) {
        std::cerr << "One or more tests failed.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
