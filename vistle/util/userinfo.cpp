#include "userinfo.h"

#include <cstdlib>

#ifdef _WIN32
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

namespace vistle {

std::string getLoginName()
{
#ifdef _WIN32
#else
    if (const char *logname = getenv("LOGNAME")) {
        return logname;
    }
    if (const char *logname = getlogin()) {
        return logname;
    }
#endif
    return "";
}

std::string getRealName()
{
#ifdef _WIN32
#else
    setpwent();
    for (auto pw = getpwent(); pw; pw = getpwent()) {
        if (pw->pw_uid == getuid()) {
            endpwent();
            if (pw->pw_gecos)
                return pw->pw_gecos;
            else
                break;
        }
    }
#endif
    return "";
}

unsigned long getUserId()
{
#ifdef _WIN32
#else
    return getuid();
#endif
    return 0;
}

} // namespace vistle
