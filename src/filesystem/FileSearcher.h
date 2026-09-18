#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sde {

class FileSearcher {
public:
    FileSearcher(std::filesystem::path rootPath, std::filesystem::path outputRoot);
    std::vector<std::filesystem::path> findExcelFiles() const;
    static bool isExcelFile(const std::filesystem::path& path);
    static bool shouldSkipDirectory(const std::filesystem::path& path, const std::filesystem::path& outputRoot);

private:
    std::filesystem::path rootPath_;
    std::filesystem::path outputRoot_;
};

} // namespace sde
