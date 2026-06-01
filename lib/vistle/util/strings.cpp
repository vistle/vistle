#include "strings.h"

#include <boost/locale/conversion.hpp>

namespace vistle {

std::string to_lower(const std::string &s)
{
    return boost::locale::to_lower(s);
}

std::string to_upper(const std::string &s)
{
    return boost::locale::to_upper(s);
}

} // namespace vistle
