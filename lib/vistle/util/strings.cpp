#include "strings.h"

#include <boost/locale/conversion.hpp>
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

} // namespace vistle
