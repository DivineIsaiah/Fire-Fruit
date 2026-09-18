#include "filesystem/DirectoryBrowser.h"

#include <algorithm>
#include <iostream>
#include <system_error>

namespace sde {

DirectoryBrowser::DirectoryBrowser(std::filesystem::path startPath)
    : currentPath_(std::filesystem::absolute(startPath)) {
    if (!std::filesystem::exists(currentPath_) || !std::filesystem::is_directory(currentPath_)) {
        return;
    }

    std::vector<std::filesystem::path> entries;
    for (const auto& entry : std::filesystem::directory_iterator(currentPath_)) {
        if (entry.is_directory()) {
            entries.push_back(entry.path());
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.filename().string() < b.filename().string();
    });
    subdirectories_ = std::move(entries);
}

std::filesystem::path DirectoryBrowser::currentDirectory() const {
    return currentPath_;
}

std::vector<std::filesystem::path> DirectoryBrowser::listSubdirectories() const {
    return subdirectories_;
}

std::optional<std::filesystem::path> DirectoryBrowser::navigateInto(size_t index) const {
    if (index >= subdirectories_.size()) {
        return std::nullopt;
    }
    return subdirectories_[index];
}

std::optional<std::filesystem::path> DirectoryBrowser::navigateBack() const {
    if (currentPath_ == currentPath_.root_path()) {
        return std::nullopt;
    }
    auto parent = currentPath_.parent_path();
    if (std::filesystem::exists(parent) && std::filesystem::is_directory(parent)) {
        return parent;
    }
    return std::nullopt;
}

void DirectoryBrowser::displayMenu() const {
    std::cout << "\nCurrent folder:\n" << currentPath_.string() << "\n\n";
    std::cout << "Available folders:\n";
    if (subdirectories_.empty()) {
        std::cout << "  (No subdirectories)\n";
        return;
    }
    for (size_t i = 0; i < subdirectories_.size(); ++i) {
        std::cout << "[" << (i + 1) << "] " << subdirectories_[i].filename().string() << "\n";
    }
    std::cout << "[" << (subdirectories_.size() + 1) << "] Search this folder\n";
    std::cout << "[" << (subdirectories_.size() + 2) << "] Go back\n";
    std::cout << "[" << (subdirectories_.size() + 3) << "] Exit\n";
}

bool DirectoryBrowser::isValid() const {
    return std::filesystem::exists(currentPath_) && std::filesystem::is_directory(currentPath_);
}

} // namespace sde
