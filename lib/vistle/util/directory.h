#ifndef VISTLE_UTIL_DIRECTORY_H
#define VISTLE_UTIL_DIRECTORY_H

#include <string>
#include <map>

#include "export.h"

namespace vistle {

class V_UTILEXPORT Directory {
public:
    Directory(int argc, char *argv[]);
    Directory(const std::string &prefix, const std::string &buildtype);
    std::string prefix() const;
    std::string bin() const;
    std::string module() const;
    std::string moduleplugin() const;
    std::string share() const;
    std::string buildType() const;

    std::string covisedir() const;

private:
    std::string m_prefix;
    std::string m_buildType;
    std::string m_buildTypeSuffix;
};

namespace directory {

V_UTILEXPORT std::string build_type();
V_UTILEXPORT std::string share(const std::string &prefix);
V_UTILEXPORT std::string configHome(); //config directory of the user

V_UTILEXPORT bool setVistleRoot(const std::string &vistleRootDir, const std::string &buildtype);
V_UTILEXPORT bool setEnvironment(const std::string &prefix);

} // namespace directory

} // namespace vistle

#endif
