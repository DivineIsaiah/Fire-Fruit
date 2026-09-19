#include "utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace sde {

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool equalsIgnoreCase(const std::string& left, const std::string& right) {
    return toLower(trim(left)) == toLower(trim(right));
}

std::string sanitizeFilename(const std::string& value) {
    std::string out;
    for (char ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|') {
            out += '_';
        } else {
            out += ch;
        }
    }
    return trim(out);
}

std::string normalizeHeaderName(const std::string& value) {
    std::string normalized = trim(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    std::string cleaned;
    cleaned.reserve(normalized.size());
    for (char ch : normalized) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            cleaned += ch;
        } else if (ch == ' ' || ch == '_' || ch == '-' || ch == '.' || ch == '/' || ch == '\\' || ch == '#') {
            // Ignore punctuation and spacing differences when comparing headers.
            continue;
        }
    }

    return trim(cleaned);
}

std::string valueToString(const std::string& value) {
    return trim(value);
}

} // namespace sde
