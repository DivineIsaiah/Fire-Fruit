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

void createTypedWorkbook(const std::filesystem::path& path) {
    OpenXLSX::XLDocument doc;
    doc.create(path.string());
    auto wks = doc.workbook().worksheet("Sheet1");

    wks.cell(1, 1).value() = "Matric No";
    wks.cell(1, 2).value() = "Integer";
    wks.cell(1, 3).value() = "Float";
    wks.cell(1, 4).value() = "String";
    wks.cell(1, 5).value() = "Empty";
    wks.cell(2, 1).value() = "DE.2020/10256";
    wks.cell(2, 2).value() = static_cast<int64_t>(8);
    wks.cell(2, 3).value() = 8.5;
    wks.cell(2, 4).value() = "47.25";
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
    const std::filesystem::path realWorkbookPath = R"(C:\Users\USER\Documents\DEV n STUFF\PROJECTS\Pesonal Projects\RESULTS\20-21\YEAR 1\FIRST SEMESTER\BASIC_MEDICAL_SCIENCES_MEDICINE_AND_SURGERY._2020_2021_1_AEB141Animal_Biology_I.xlsx)";

    std::filesystem::path workbookPath = realWorkbookPath;
    std::filesystem::path root = std::filesystem::temp_directory_path() / "sde_extractor_test";
    std::filesystem::path outputsRoot = root / "OUTPUTS";
    if (!std::filesystem::exists(realWorkbookPath)) {
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "RESULTS");
        std::filesystem::create_directories(outputsRoot);

        const std::vector<std::vector<std::string>> rows = {
            {"Matric No", "Name", "CA", "Exam", "Total", "Grade"},
            {"MIVA/CSC/21/1234", "Divine Isaiah", "28", "55", "83", "A"},
            {"MIVA/CSC/21/1234", "Divine Isaiah", "30", "60", "90", "A"},
            {"MIVA/CSC/21/9999", "Someone Else", "10", "20", "30", "C"}
        };
        workbookPath = root / "RESULTS" / "CSC301.xlsx";
        createWorkbook(workbookPath, rows);
    }

    sde::ApplicationConfig config;
    config.appRoot = root;
    config.resultsRoot = workbookPath.parent_path();
    config.outputsRoot = outputsRoot;

    const std::string targetMatric = !std::filesystem::exists(realWorkbookPath) ? "MIVA/CSC/21/1234" : "DE.2020/10256";
    sde::StudentExtractor extractor(config, targetMatric);
    auto result = extractor.processWorkbook(workbookPath);

    if (result.records.empty()) {
        std::filesystem::remove_all(root);
        return false;
    }

    if (std::filesystem::exists(realWorkbookPath)) {
        auto& record = result.records.front();
        const auto testOne = record.fields.find("TEST 1");
        const auto totalScore = record.fields.find("Total Score");
        const auto letterGrade = record.fields.find("Letter Grade");

        if (testOne == record.fields.end() || totalScore == record.fields.end() || letterGrade == record.fields.end()) {
            std::filesystem::remove_all(root);
            return false;
        }

        if (testOne->second != "8" || totalScore->second != "47" || letterGrade->second != "D") {
            std::filesystem::remove_all(root);
            return false;
        }
    } else {
        if (result.records.size() != 2) {
            std::filesystem::remove_all(root);
            return false;
        }

        if (result.records[0].fields["Name"] != "Divine Isaiah") {
            std::filesystem::remove_all(root);
            return false;
        }

        if (result.records[1].fields["CA"] != "30") {
            std::filesystem::remove_all(root);
            return false;
        }

        if (result.records[0].fields["Exam"] != "55" || result.records[0].fields["Total"] != "83" || result.records[0].fields["Grade"] != "A") {
            std::filesystem::remove_all(root);
            return false;
        }
    }

    std::filesystem::remove_all(root);
    return true;
}

bool testDuplicateAndBlankHeaderHandling() {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "sde_duplicate_header_test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "RESULTS");
    std::filesystem::create_directories(root / "OUTPUTS");

    const std::vector<std::vector<std::string>> rows = {
        {"Matric No", "Subject", "Subject", "", "Total Score", "Testing"},
        {"DE.2020/10256", "Physiology", "Biochemistry", "extra value", "82", "pass"},
        {"DE.2020/99999", "Elsewhere", "Other", "skip me", "33", "fail"}
    };

    const auto workbookPath = root / "RESULTS" / "AEB141.xlsx";
    createWorkbook(workbookPath, rows);

    sde::ApplicationConfig config;
    config.appRoot = root;
    config.resultsRoot = root / "RESULTS";
    config.outputsRoot = root / "OUTPUTS";

    sde::StudentExtractor extractor(config, "DE.2020/10256");
    const auto result = extractor.processWorkbook(workbookPath);

    if (result.records.size() != 1) {
        std::filesystem::remove_all(root);
        return false;
    }

    const auto& record = result.records.front();
    if (record.fields.find("Subject") == record.fields.end()) {
        std::filesystem::remove_all(root);
        return false;
    }

    const auto subjectSourceIt = record.fields.find("Subject (source)");
    const auto subjectSource2It = record.fields.find("Subject (source 2)");
    if (subjectSourceIt == record.fields.end() || subjectSource2It == record.fields.end()) {
        std::filesystem::remove_all(root);
        return false;
    }

    if (subjectSourceIt->second != "Physiology" || subjectSource2It->second != "Biochemistry") {
        std::filesystem::remove_all(root);
        return false;
    }

    const auto blankHeaderIt = record.fields.find("Column 4");
    if (blankHeaderIt == record.fields.end() || blankHeaderIt->second != "extra value") {
        std::filesystem::remove_all(root);
        return false;
    }

    std::filesystem::remove_all(root);
    return true;
}

bool testCellValueConversion() {
    const auto root = std::filesystem::temp_directory_path() / "sde_cell_value_test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "RESULTS");
    std::filesystem::create_directories(root / "OUTPUTS");

    const auto workbookPath = root / "RESULTS" / "typed.xlsx";
    createTypedWorkbook(workbookPath);

    sde::ApplicationConfig config;
    config.appRoot = root;
    config.resultsRoot = root / "RESULTS";
    config.outputsRoot = root / "OUTPUTS";

    sde::StudentExtractor extractor(config, "DE.2020/10256");
    const auto result = extractor.processWorkbook(workbookPath);
    if (result.records.size() != 1) {
        std::filesystem::remove_all(root);
        return false;
    }

    const auto& fields = result.records.front().fields;
    const auto integerIt = fields.find("Integer");
    const auto floatIt = fields.find("Float");
    const auto stringIt = fields.find("String");
    if (integerIt == fields.end() || floatIt == fields.end() || stringIt == fields.end()) {
        std::filesystem::remove_all(root);
        return false;
    }

    const bool valuesOk = integerIt->second == "8" && floatIt->second == "8.5" && stringIt->second == "47.25" && fields.find("Empty") == fields.end();
    std::filesystem::remove_all(root);
    return valuesOk;
}

} // namespace

int main() {
    const bool stringOk = testStringUtils();
    const bool fileOk = testFileSearcher();
    const bool extractorOk = testStudentExtractor();
    const bool collisionOk = testDuplicateAndBlankHeaderHandling();
    const bool conversionOk = testCellValueConversion();

    if (!stringOk || !fileOk || !extractorOk || !collisionOk || !conversionOk) {
        std::cerr << "One or more tests failed.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
