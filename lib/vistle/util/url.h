#ifndef VISTLE_UTIL_URL_H
#define VISTLE_UTIL_URL_H

#include "export.h"
#include <string>
#include <map>

namespace vistle {

class V_UTILEXPORT Url {
public:
    Url(const std::string &url);
    static Url fromFileOrUrl(const std::string &furl);
    static std::string decode(const std::string &str, bool path = false);

    std::string str() const;
    operator std::string() const;

    std::string extension() const;
    bool valid() const;
    bool isLocal() const;

    const std::string &scheme() const;
    const std::string &authority() const;
    const std::string &userinfo() const;
    const std::string &host() const;
    const std::string &port() const;
    const std::string &path() const;
    const std::map<std::string, std::string> &query() const;
    std::pair<std::string, bool> query(const std::string &key) const;
    const std::string &fragment() const;

private:
    bool m_valid = false;

    std::string m_scheme;
    std::string m_authority;
    bool m_haveAuthority = false;
    std::string m_userinfo;
    std::string m_host;
    std::string m_port;
    std::string m_path;
    std::map<std::string, std::string> m_query;
    std::string m_fragment;

    Url();
};

} // namespace vistle
#endif
