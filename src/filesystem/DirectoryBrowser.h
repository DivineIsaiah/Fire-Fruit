#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sde {

class DirectoryBrowser {
public:
    explicit DirectoryBrowser(std::filesystem::path startPath);

    std::filesystem::path currentDirectory() const;
    std::vector<std::filesystem::path> listSubdirectories() const;
    std::optional<std::filesystem::path> navigateInto(size_t index) const;
    std::optional<std::filesystem::path> navigateBack() const;
    void displayMenu() const;
    bool isValid() const;

private:
    std::filesystem::path currentPath_;
    std::vector<std::filesystem::path> subdirectories_;
};

} // namespace sde
