#include "strings.h"

#include <boost/locale/conversion.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <algorithm>

namespace vistle {

std::string to_lower(const std::string &s)
{
    try {
        return boost::locale::to_lower(s);
    } catch (std::bad_cast &e) {
    }

    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
    return result;
}

std::string to_upper(const std::string &s)
{
    try {
        return boost::locale::to_upper(s);
    } catch (std::bad_cast &e) {
    }

    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
    return result;
}

std::string escape(const std::string &s)
{
    std::string result;
    for (auto c: s) {
        switch (c) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        default:
            if (std::isprint(c)) {
                result += c;
            } else {
                result += str(boost::format("\\x%02x") % static_cast<int>(c));
            }
        }
    }
    return result;
}

} // namespace vistle
