#include "str.h"

namespace vistle {

/**
 * @brief: Checks if suffix string is the end of main.
 *
 * @param main: main string.
 * @param suffix: suffix string
 *
 * @return: True if given suffix is end of main.
 */
bool checkFileEnding(const std::string &main, const std::string &suffix)
{
    if (main.size() >= suffix.size())
        if (main.compare(main.size() - suffix.size(), suffix.size(), suffix) == 0)
            return true;
    return false;
}

} // namespace vistle
