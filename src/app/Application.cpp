#include "app/Application.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "excel/ExcelWriter.h"
#include "extraction/StudentExtractor.h"
#include "filesystem/DirectoryBrowser.h"
#include "filesystem/FileSearcher.h"
#include "utils/StringUtils.h"

namespace sde {

Application::Application()
    : config_(ApplicationConfig{}), logger_(resolveLogsRoot()) {

    config_.appRoot = resolveApplicationRoot();
    config_.resultsRoot = resolveResultsRoot();
    config_.outputsRoot = ensureDirectoryExists(resolveOutputsRoot());
    config_.logsRoot = resolveLogsRoot();

    ensureDirectoryExists(config_.appRoot);
    ensureDirectoryExists(config_.resultsRoot);
    ensureDirectoryExists(config_.outputsRoot);
    ensureDirectoryExists(config_.logsRoot);

    logger_ = Logger(config_.logsRoot);

    logger_.log("Session started.");
    logger_.log("Application root: " + config_.appRoot.string());
    logger_.log("RESULTS root: " + config_.resultsRoot.string());
    logger_.log("OUTPUTS root: " + config_.outputsRoot.string());
}

std::filesystem::path Application::resolveApplicationRoot() const {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");

    if (localAppData != nullptr && *localAppData != '\0') {
        return std::filesystem::path(localAppData)
            / "StudentDataExtractor";
    }
#endif

    return std::filesystem::absolute(
        std::filesystem::current_path()
    );
}

std::filesystem::path Application::resolveResultsRoot() const {
    const auto resultsRoot = config_.appRoot / "RESULTS";

    return resultsRoot;
}

std::filesystem::path Application::resolveOutputsRoot() const {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");

    if (localAppData != nullptr && *localAppData != '\0') {
        return std::filesystem::path(localAppData)
            / "StudentDataExtractor"
            / "OUTPUTS";
    }
#endif

    return std::filesystem::absolute(
        std::filesystem::current_path() / "OUTPUTS"
    );
}

std::filesystem::path Application::resolveLogsRoot() const {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");

    if (localAppData != nullptr && *localAppData != '\0') {
        return std::filesystem::path(localAppData)
            / "StudentDataExtractor"
            / "logs";
    }
#endif

    return std::filesystem::absolute(
        std::filesystem::current_path() / "logs"
    );
}

std::filesystem::path Application::ensureDirectoryExists(
    const std::filesystem::path& path
) const {
    if (!path.empty()) {
        std::error_code error;
        std::filesystem::create_directories(path, error);

        if (error) {
            std::cerr << "Failed to create directory: "
                      << path
                      << "\nError: "
                      << error.message()
                      << '\n';
        }
    }

    return path;
}

std::vector<std::filesystem::path> Application::browseDirectories() const {
    std::vector<std::filesystem::path> output;

    std::filesystem::path start = config_.resultsRoot;

    std::vector<std::filesystem::path> stack = {start};

    while (!stack.empty()) {
        auto p = stack.back();
        stack.pop_back();

        if (!std::filesystem::exists(p) ||
            !std::filesystem::is_directory(p)) {
            continue;
        }

        output.push_back(p);

        std::error_code error;

        for (const auto& entry :
             std::filesystem::directory_iterator(p, error)) {

            if (error) {
                break;
            }

            if (entry.is_directory(error)) {
                stack.push_back(entry.path());
            }
        }
    }

    std::sort(output.begin(), output.end());

    return output;
}

std::filesystem::path Application::promptForFolderSelection() const {
    std::filesystem::path selected = config_.resultsRoot;

    while (true) {
        DirectoryBrowser browser(selected);

        if (!browser.isValid()) {
            std::cout << "The selected directory is not available.\n";
            break;
        }

        browser.displayMenu();

        std::cout << "\nSelect an option: ";

        int choice = 0;
        std::cin >> choice;

        if (choice >= 1 &&
            choice <= static_cast<int>(
                browser.listSubdirectories().size())) {

            auto child =
                browser.navigateInto(
                    static_cast<size_t>(choice - 1)
                );

            if (child.has_value()) {
                selected = *child;
                continue;
            }

        } else if (
            choice ==
            static_cast<int>(
                browser.listSubdirectories().size()) + 1) {

            return selected;

        } else if (
            choice ==
            static_cast<int>(
                browser.listSubdirectories().size()) + 2) {

            if (selected.parent_path() != selected) {
                auto parent = browser.navigateBack();

                if (parent.has_value()) {
                    selected = *parent;
                    continue;
                }
            }

        } else if (
            choice ==
            static_cast<int>(
                browser.listSubdirectories().size()) + 3) {

            std::exit(0);
        }

        std::cout << "Invalid selection.\n";
    }

    return config_.resultsRoot;
}

std::filesystem::path Application::promptForSearchRoot() const {
    std::cout << "[1] Browse RESULTS folders\n";
    std::cout << "[2] Search entire RESULTS folder\n";
    std::cout << "[3] Exit\n";
    std::cout << "Select an option: ";

    int choice = 0;
    std::cin >> choice;

    if (choice == 1) {
        return promptForFolderSelection();
    }

    if (choice == 2) {
        return config_.resultsRoot;
    }

    std::exit(0);

    return config_.resultsRoot;
}

std::string Application::promptForMatricNumber() const {
    std::string matric;

    std::cout << "Enter matriculation number: ";

    std::cin >> matric;

    matric = trim(matric);

    return matric;
}

std::vector<std::filesystem::path>
Application::discoverExcelFiles(
    const std::filesystem::path& root
) const {

    FileSearcher searcher(
        root,
        config_.outputsRoot
    );

    auto files = searcher.findExcelFiles();

    std::cout << "Discovered "
              << files.size()
              << " Excel files.\n";

    logger_.log(
        "Discovered " +
        std::to_string(files.size()) +
        " Excel files in " +
        root.string()
    );

    return files;
}

ExtractionNotification Application::extractStudent(
    const std::filesystem::path& root,
    const std::string& matricNumber
) const {

    ExtractionNotification result;

    auto files = discoverExcelFiles(root);

    result.processed =
        static_cast<int>(files.size());

    logger_.log(
        "Processing matriculation number: " +
        matricNumber
    );

    StudentExtractor extractor(
        config_,
        matricNumber
    );

    for (const auto& file : files)
    {
        auto fileResult =
            extractor.processWorkbook(file);

        result.records.insert(
            result.records.end(),
            fileResult.records.begin(),
            fileResult.records.end()
        );

        result.warnings.insert(
            result.warnings.end(),
            fileResult.warnings.begin(),
            fileResult.warnings.end()
        );

        result.errors.insert(
            result.errors.end(),
            fileResult.errors.begin(),
            fileResult.errors.end()
        );

        // Log workbook-level errors immediately
        if (!fileResult.errors.empty()) {
        for (const auto& error : fileResult.errors) {
            logger_.log(
                "WORKBOOK ERROR: " +
                error.filePath +
                " - " +
                error.reason
            );
        }
    }

        result.matches +=
            fileResult.matches;

        result.noMatch +=
            fileResult.noMatch;
    }

    if (!result.records.empty()) {

        logger_.log(
            "Records found: " +
            std::to_string(result.records.size())
        );

        logger_.log(
            "Starting output workbook generation."
        );

        try {
            ExcelWriter writer(
                config_.outputsRoot
            );

            logger_.log(
                "ExcelWriter created successfully."
            );

            auto outputFile =
                writer.writeWorkbook(
                    result.records,
                    matricNumber
                );

            logger_.log(
                "writeWorkbook completed successfully."
            );

            result.outputPath =
                outputFile.string();

            logger_.log(
                "Output generated: " +
                outputFile.string()
            );
        }
        catch (const std::exception& ex) {
            logger_.log(
                "OUTPUT GENERATION EXCEPTION: " +
                std::string(ex.what())
            );

            throw;
        }
    }
    else {

    logger_.log(
        "No records found. Output workbook will not be generated."
    );
}

    result.processed =
        static_cast<int>(files.size());

    result.matches =
        static_cast<int>(result.records.size());

    return result;
}

void Application::printSummary(
    const ExtractionNotification& result
) const {

    std::cout
        << "\n========================================\n";

    std::cout
        << "          EXTRACTION COMPLETE\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Files found:       "
        << result.processed
        << "\n";

    std::cout
        << "Matching files:    "
        << result.matches
        << "\n";

    std::cout
        << "No matches:        "
        << result.noMatch
        << "\n";

    std::cout
        << "Errors:            "
        << result.errors.size()
        << "\n";

    std::cout
        << "Records extracted: "
        << result.records.size()
        << "\n";

    std::cout
        << "Output:            "
        << result.outputPath
        << "\n";
}

std::string Application::formatOutputName(
    const std::string& matricNumber
) {

    auto safe =
        sanitizeFilename(matricNumber);

    std::replace(
        safe.begin(),
        safe.end(),
        '/',
        '_'
    );

    std::replace(
        safe.begin(),
        safe.end(),
        '\\',
        '_'
    );

    std::replace(
        safe.begin(),
        safe.end(),
        ':',
        '_'
    );

    return safe.empty()
        ? "student"
        : safe;
}

void Application::run() {

    while (true) {

        std::cout
            << "========================================\n";

        std::cout
            << "       STUDENT DATA EXTRACTOR\n";

        std::cout
            << "========================================\n";

        auto chosenRoot =
            promptForSearchRoot();

        auto matric =
            promptForMatricNumber();

        if (matric.empty()) {

            std::cout
                << "Matriculation number cannot be empty.\n";

            continue;
        }

        auto result =
            extractStudent(
                chosenRoot,
                matric
            );

        printSummary(result);

        std::cout
            << "\n[1] Start another extraction\n"
            << "[2] Exit\n";

        std::cout
            << "Select an option: ";

        int choice = 0;

        std::cin >> choice;

        if (choice != 1) {
            break;
        }
    }
}

} // namespace sde