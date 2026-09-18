#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "models/ApplicationConfig.h"
#include "models/StudentRecord.h"
#include "utils/Logger.h"

namespace sde {

class Application {
public:
    Application();
    void run();

private:
    std::filesystem::path resolveApplicationRoot() const;
    std::filesystem::path resolveResultsRoot() const;
    std::filesystem::path resolveOutputsRoot() const;
    std::filesystem::path resolveLogsRoot() const;
    std::filesystem::path ensureDirectoryExists(const std::filesystem::path& path) const;
    std::vector<std::filesystem::path> browseDirectories() const;
    std::filesystem::path promptForFolderSelection() const;
    std::filesystem::path promptForSearchRoot() const;
    std::string promptForMatricNumber() const;
    std::vector<std::filesystem::path> discoverExcelFiles(const std::filesystem::path& root) const;
    ExtractionNotification extractStudent(const std::filesystem::path& root, const std::string& matricNumber) const;
    void printSummary(const ExtractionNotification& result) const;
    static std::string formatOutputName(const std::string& matricNumber);

    ApplicationConfig config_;
    Logger logger_;
};

} // namespace sde
