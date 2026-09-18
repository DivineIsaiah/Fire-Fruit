#include "filesystem/FileSearcher.h"

#include <algorithm>
#include <string>

namespace sde {

FileSearcher::FileSearcher(std::filesystem::path rootPath, std::filesystem::path outputRoot)
    : rootPath_(std::filesystem::absolute(rootPath)), outputRoot_(std::filesystem::absolute(outputRoot)) {}

bool FileSearcher::isExcelFile(const std::filesystem::path& path) {
    if (!path.has_extension()) {
        return false;
    }
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return ext == ".XLSX" || ext == ".XLS";
}

bool FileSearcher::shouldSkipDirectory(const std::filesystem::path& path, const std::filesystem::path& outputRoot) {
    std::filesystem::path absPath = std::filesystem::absolute(path);
    std::error_code ec;
    auto pathStr = absPath.lexically_normal().string();
    auto outputStr = std::filesystem::absolute(outputRoot).lexically_normal().string();
    return pathStr == outputStr || pathStr.find(outputStr + "\\") == 0;
}

std::vector<std::filesystem::path> FileSearcher::findExcelFiles() const {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(rootPath_) || !std::filesystem::is_directory(rootPath_)) {
        return files;
    }

    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(rootPath_, std::filesystem::directory_options::skip_permission_denied, ec);
    std::filesystem::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_directory()) {
            if (shouldSkipDirectory(entry.path(), outputRoot_)) {
                it.disable_recursion_pending();
            }
            continue;
        }

        if (entry.is_regular_file() && isExcelFile(entry.path())) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

} // namespace sde
