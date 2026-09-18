#pragma once

#include <string>
#include <vector>

namespace sde {

std::string trim(const std::string& value);
std::string toLower(std::string value);
bool equalsIgnoreCase(const std::string& left, const std::string& right);
std::string sanitizeFilename(const std::string& value);
std::string normalizeHeaderName(const std::string& value);
std::string valueToString(const std::string& value);

} // namespace sde
