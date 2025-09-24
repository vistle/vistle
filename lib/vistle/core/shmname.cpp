#include "shmname.h"

#include <cstring>
#include <cassert>

namespace vistle {

shm_name_t::shm_name_t(const std::string &s)
{
    size_t len = s.length();
    assert(len < name.size());
    if (len >= name.size())
        len = name.size() - 1;
    strncpy(name.data(), s.data(), len);
    name[len] = '\0';
}

std::string shm_name_t::str() const
{
    return name.data();
}

shm_name_t::operator const char *() const
{
    return name.data();
}

shm_name_t::operator char *()
{
    return name.data();
}

shm_name_t::operator std::string() const
{
    return str();
}

bool shm_name_t::operator==(const std::string &rhs) const
{
    return rhs == name.data();
}

bool shm_name_t::operator==(const shm_name_t &rhs) const
{
    return !strncmp(name.data(), rhs.name.data(), name.size());
}

bool shm_name_t::empty() const
{
    return name.data()[0] == '\0' || !strcmp(name.data(), "INVALID");
}

void shm_name_t::clear()
{
    name.data()[0] = '\0';
}

std::string operator+(const std::string &s, const shm_name_t &n)
{
    return s + n.name.data();
}

} // namespace vistle
