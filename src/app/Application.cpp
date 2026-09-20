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
    logger_ = Logger(config_.logsRoot);
    logger_.log("Session started.");
    logger_.log("Application root: " + config_.appRoot.string());
    logger_.log("RESULTS root: " + config_.resultsRoot.string());
    logger_.log("OUTPUTS root: " + config_.outputsRoot.string());
}

std::filesystem::path Application::resolveApplicationRoot() const {
    std::filesystem::path current = std::filesystem::absolute(std::filesystem::current_path());
    auto candidate = current;
    while (true) {
        if (std::filesystem::exists(candidate / "RESULTS") || std::filesystem::exists(candidate / "OUTPUTS")) {
            return candidate;
        }
        auto parent = candidate.parent_path();
        if (parent == candidate) {
            break;
        }
        candidate = parent;
    }
    return std::filesystem::absolute(std::filesystem::current_path());
}

std::filesystem::path Application::resolveResultsRoot() const {
    const auto appRoot = config_.appRoot;
    auto expected = appRoot / "RESULTS";
    if (std::filesystem::exists(expected) && std::filesystem::is_directory(expected)) {
        return expected;
    }
    auto current = std::filesystem::absolute(std::filesystem::current_path());
    while (true) {
        auto candidate = current / "RESULTS";
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
            return candidate;
        }
        auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    return appRoot / "RESULTS";
}

std::filesystem::path Application::resolveOutputsRoot() const {
    std::filesystem::path appRoot = config_.appRoot;
    std::filesystem::path expected = appRoot / "OUTPUTS";
    if (std::filesystem::exists(expected) && std::filesystem::is_directory(expected)) {
        return expected;
    }
    std::filesystem::path current = std::filesystem::absolute(std::filesystem::current_path());
    while (true) {
        auto candidate = current / "OUTPUTS";
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
            return candidate;
        }
        auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData != nullptr && *localAppData != '\0' &&
        !std::filesystem::exists(appRoot / "CMakeLists.txt")) {
        return std::filesystem::path(localAppData) / "StudentDataExtractor" / "OUTPUTS";
    }
    return expected;
}

std::filesystem::path Application::resolveLogsRoot() const {
    std::filesystem::path appRoot = resolveApplicationRoot();
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData != nullptr && *localAppData != '\0' &&
        !std::filesystem::exists(appRoot / "CMakeLists.txt")) {
        return std::filesystem::path(localAppData) / "StudentDataExtractor" / "logs";
    }
    return appRoot / "logs";
}

std::filesystem::path Application::ensureDirectoryExists(const std::filesystem::path& path) const {
    if (!path.empty()) {
        std::filesystem::create_directories(path);
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
        if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p)) {
            continue;
        }
        output.push_back(p);
        for (const auto& entry : std::filesystem::directory_iterator(p)) {
            if (entry.is_directory()) {
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

        if (choice >= 1 && choice <= static_cast<int>(browser.listSubdirectories().size())) {
            auto child = browser.navigateInto(static_cast<size_t>(choice - 1));
            if (child.has_value()) {
                selected = *child;
                continue;
            }
        } else if (choice == static_cast<int>(browser.listSubdirectories().size()) + 1) {
            return selected;
        } else if (choice == static_cast<int>(browser.listSubdirectories().size()) + 2) {
            if (selected.parent_path() != selected) {
                auto parent = browser.navigateBack();
                if (parent.has_value()) {
                    selected = *parent;
                    continue;
                }
            }
        } else if (choice == static_cast<int>(browser.listSubdirectories().size()) + 3) {
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

std::vector<std::filesystem::path> Application::discoverExcelFiles(const std::filesystem::path& root) const {
    FileSearcher searcher(root, config_.outputsRoot);
    auto files = searcher.findExcelFiles();
    std::cout << "Discovered " << files.size() << " Excel files.\n";
    logger_.log("Discovered " + std::to_string(files.size()) + " Excel files in " + root.string());
    return files;
}

ExtractionNotification Application::extractStudent(const std::filesystem::path& root, const std::string& matricNumber) const {
    ExtractionNotification result;
    auto files = discoverExcelFiles(root);
    result.processed = static_cast<int>(files.size());
    logger_.log("Processing matriculation number: " + matricNumber);

    StudentExtractor extractor(config_, matricNumber);
    for (const auto& file : files) {
        try {
            auto fileResult = extractor.processWorkbook(file);
            result.records.insert(result.records.end(), fileResult.records.begin(), fileResult.records.end());
            result.warnings.insert(result.warnings.end(), fileResult.warnings.begin(), fileResult.warnings.end());
            result.errors.insert(result.errors.end(), fileResult.errors.begin(), fileResult.errors.end());
            result.matches += fileResult.matches;
            result.noMatch += fileResult.noMatch;
        } catch (const std::exception& ex) {
            result.errors.push_back({file.string(), ex.what()});
            logger_.log("Workbook processing failed: " + file.string() + " - " + ex.what());
        }
    }

    if (!result.records.empty()) {
        try {
            logger_.log("Writing output workbook to: " + config_.outputsRoot.string());
            ExcelWriter writer(config_.outputsRoot);
            auto outputFile = writer.writeWorkbook(result.records, matricNumber);
            result.outputPath = outputFile.string();
            logger_.log("Output generated: " + outputFile.string());
        } catch (const std::exception& ex) {
            result.errors.push_back({config_.outputsRoot.string(), ex.what()});
            logger_.log("Output generation failed: " + std::string(ex.what()));
        }
    }

    result.processed = static_cast<int>(files.size());
    result.matches = static_cast<int>(result.records.size());
    return result;
}

void Application::printSummary(const ExtractionNotification& result) const {
    std::cout << "\n========================================\n";
    std::cout << "          EXTRACTION COMPLETE\n";
    std::cout << "========================================\n";
    std::cout << "Files found:       " << result.processed << "\n";
    std::cout << "Matching files:    " << result.matches << "\n";
    std::cout << "No matches:        " << result.noMatch << "\n";
    std::cout << "Errors:            " << result.errors.size() << "\n";
    std::cout << "Records extracted: " << result.records.size() << "\n";
    std::cout << "Output:            " << result.outputPath << "\n";
}

std::string Application::formatOutputName(const std::string& matricNumber) {
    auto safe = sanitizeFilename(matricNumber);
    std::replace(safe.begin(), safe.end(), '/', '_');
    std::replace(safe.begin(), safe.end(), '\\', '_');
    std::replace(safe.begin(), safe.end(), ':', '_');
    return safe.empty() ? "student" : safe;
}

void Application::run() {
    while (true) {
        std::cout << "========================================\n";
        std::cout << "       STUDENT DATA EXTRACTOR\n";
        std::cout << "========================================\n";

        auto chosenRoot = promptForSearchRoot();
        auto matric = promptForMatricNumber();
        if (matric.empty()) {
            std::cout << "Matriculation number cannot be empty.\n";
            continue;
        }

        auto result = extractStudent(chosenRoot, matric);
        printSummary(result);

        std::cout << "\n[1] Start another extraction\n[2] Exit\n";
        std::cout << "Select an option: ";
        int choice = 0;
        std::cin >> choice;
        if (choice != 1) {
            break;
        }
    }
}

} // namespace sde
