#ifndef VISTLE_CORE_SHMNAME_H
#define VISTLE_CORE_SHMNAME_H

#include "export.h"
#include <array>
#include <string>
#include "archives_config.h"

namespace vistle {

struct V_COREEXPORT shm_name_t {
    std::array<char, 32> name;
    //shm_name_t(const std::string &s = "INVALID");
    shm_name_t(const std::string &s = std::string());

    operator const char *() const;
    operator char *();
    operator std::string() const;
    std::string str() const;
    bool operator==(const std::string &rhs) const;
    bool operator==(const shm_name_t &rhs) const;
    bool empty() const;
    void clear();

    ARCHIVE_ACCESS_SPLIT
    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);
};
std::string operator+(const std::string &s, const shm_name_t &n);

} // namespace vistle
#endif
