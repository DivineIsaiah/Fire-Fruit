#include "extraction/ColumnMatcher.h"

#include <algorithm>
#include <cctype>

#include "utils/StringUtils.h"

namespace sde {

std::vector<std::string> ColumnMatcher::defaultAliases() {
    return {
        "matric number", "matric no", "matric no.", "matriculation number",
        "matriculation no", "registration number", "reg no", "reg. no.", "student id",
        "studentid", "matriculation", "student number"
    };
}

bool ColumnMatcher::isMatricColumnName(const std::string& header, const std::vector<std::string>& aliases) {
    const auto normalized = normalizeHeaderName(header);
    const auto names = aliases.empty() ? defaultAliases() : aliases;
    for (const auto& alias : names) {
        if (normalizeHeaderName(alias) == normalized) {
            return true;
        }
    }
    return false;
}

bool ColumnMatcher::matchesMatricValue(const std::string& left, const std::string& right) {
    return equalsIgnoreCase(trim(left), trim(right));
}

} // namespace sde
