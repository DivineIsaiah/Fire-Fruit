#pragma once

#include <string>
#include <vector>

namespace sde {

class ColumnMatcher {
public:
    static bool isMatricColumnName(const std::string& header, const std::vector<std::string>& aliases = {});
    static bool matchesMatricValue(const std::string& left, const std::string& right);
    static std::vector<std::string> defaultAliases();
};

} // namespace sde
