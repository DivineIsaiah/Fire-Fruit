#include "utils/Logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace sde {

Logger::Logger(std::filesystem::path logDir)
    : logDir_(std::filesystem::absolute(logDir)) {
    std::filesystem::create_directories(logDir_);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &in_time_t);
#else
    localtime_r(&in_time_t, &localTime);
#endif
    std::ostringstream ss;
    ss << "extractor_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".log";
    filePath_ = logDir_ / ss.str();
}

void Logger::log(const std::string& message) const {
    std::ofstream out(filePath_, std::ios::app);
    if (!out.is_open()) {
        return;
    }
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &in_time_t);
#else
    localtime_r(&in_time_t, &localTime);
#endif
    out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << " - " << message << '\n';
}

std::filesystem::path Logger::logPath() const {
    return filePath_;
}

} // namespace sde
