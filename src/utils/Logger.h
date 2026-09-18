#pragma once

#include <filesystem>
#include <string>

namespace sde {

class Logger {
public:
    explicit Logger(std::filesystem::path logDir);
    void log(const std::string& message) const;
    std::filesystem::path logPath() const;

private:
    std::filesystem::path logDir_;
    std::filesystem::path filePath_;
};

} // namespace sde
