#include "url.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <locale>

namespace vistle {

static std::string url_decode(const std::string &str, bool in_path)
{
    std::string decoded;
    decoded.reserve(str.size());
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 3 <= str.size()) {
                int value = 0;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    decoded += static_cast<char>(value);
                    i += 2;
                } else {
                    decoded.clear();
                    break;
                }
            } else {
                decoded.clear();
                break;
            }
        } else if (in_path && str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

static std::string url_encode(const std::string &str)
{
    std::set<char> reserved{' ', '!', '#', '$', '%', '&', '\'', '(', ')', '*',
                            '+', ',', '/', ':', ';', '=', '?',  '@', '[', ']'};
    std::string encoded;
    encoded.reserve(str.size());
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (reserved.find(str[i]) == reserved.end()) {
            encoded.push_back(str[i]);
        } else {
            std::ostringstream os;
            os << std::hex << std::setw(2) << std::setfill('0') << int(str[i]);
            auto hex = os.str();
            encoded.push_back('%');
            std::copy(hex.begin(), hex.end(), std::back_inserter(encoded));
        }
    }
    return encoded;
}


Url::Url(const std::string &url)
{
    if (url.empty())
        return;

    std::locale C("C");
    auto it = url.begin();
    // must contain scheme and must begin with an alphabet character
    if (!isalpha(*it, C))
        return;

    for (; it != url.end(); ++it) {
        if (std::isalnum(*it, C))
            continue;
        if (*it == '+')
            continue;
        if (*it == '-')
            continue;
        if (*it == '.')
            continue;
        if (*it == ':')
            break;

        // weird character in URL scheme
        return;
    }
    if (it == url.end())
        return;
#ifdef WIN32
    // probably just a drive letter, not a URL scheme
    if (it - url.begin() <= 1)
        return;
#endif

    m_scheme = std::string(url.begin(), it);
    ++it;

    int numSlash = 0;
    auto authorityBegin = it;
    for (; it != url.end(); ++it) {
        if (numSlash >= 2)
            break;
        if (*it != '/')
            break;
        ++numSlash;
    }
    if (numSlash >= 2) {
        m_haveAuthority = true;
        auto slash = std::find(it, url.end(), '/');
        auto question = std::find(it, url.end(), '?');
        auto hash = std::find(it, url.end(), '#');
        auto end = slash;
        if (hash < end)
            end = hash;
        if (question < end)
            end = question;
        auto authority = std::string(it, end);
        auto at = std::find(authority.begin(), authority.end(), '@');
        auto host = authority.begin();
        if (at != authority.end()) {
            m_userinfo = decode(std::string(authority.begin(), at));
            host = at + 1;
        }
        auto colon = std::find(host, authority.end(), ':');
        if (colon != authority.end()) {
            m_host = std::string(host, colon);
            m_port = std::string(colon + 1, authority.end());
        } else {
            m_host = std::string(host, authority.end());
        }
        m_authority = decode(std::string(it, end));
        it = end;
    } else {
        // no authority
        it = authorityBegin;
    }

    auto question = std::find(it, url.end(), '?');
    auto hash = std::find(it, url.end(), '#');
    if (question == url.end()) {
        m_path = decode(std::string(it, hash), true);
    } else {
        m_path = decode(std::string(it, question), true);
        it = question;
        ++it;
        hash = std::find(it, url.end(), '#');
        {
            auto query = std::string(it, hash);
            auto keyvalue = query.begin();
            auto amp = std::find(keyvalue, query.end(), '&');
            while (keyvalue != query.end()) {
                auto eq = std::find(keyvalue, amp, '=');
                auto key = decode(std::string(keyvalue, eq));
                if (eq == amp)
                    m_query[key] = "";
                else
                    m_query[key] = decode(std::string(eq + 1, amp));
                keyvalue = amp;
                if (keyvalue != query.end())
                    ++keyvalue;
                amp = std::find(keyvalue, query.end(), '&');
            };
        }
    }
    it = hash;
    if (it != url.end())
        ++it;
    m_fragment = decode(std::string(it, url.end()));

    m_valid = true;
}

Url Url::fromFileOrUrl(const std::string &furl)
{
    Url url(furl);
    if (url.valid())
        return url;

    Url file;
    file.m_scheme = "file";
    file.m_path = furl;
    file.m_valid = true;
    return file;
}

std::string Url::decode(const std::string &str, bool path)
{
    return url_decode(str, path);
}

std::string Url::encode(const std::string &str)
{
    return url_encode(str);
}

std::string Url::str() const
{
    if (!valid())
        return "(invalid)";

    if (scheme() == "file")
        return path();

    std::string str = scheme();
    str += ":";
    if (m_haveAuthority) {
        str += "//";
        str += authority();
    }
    str += path();
    if (!m_query.empty()) {
        std::string query;
        for (const auto &kv: m_query) {
            if (!query.empty())
                query += "&";
            query += kv.first + '=' + kv.second;
        }
        str += "?" + query;
    }
    if (!m_fragment.empty()) {
        str += "#";
        str += m_fragment;
    }

    return str;
}

std::string Url::extension() const
{
    if (!m_valid)
        return std::string();
    if (m_path.empty())
        return std::string();
    auto pos = m_path.find_last_of('.');
    if (pos == std::string::npos)
        return std::string();
    return m_path.substr(pos);
}

bool Url::valid() const
{
    return m_valid;
}

bool Url::isLocal() const
{
    return scheme() == "file";
}

const std::string &Url::scheme() const
{
    return m_scheme;
}

const std::string &Url::authority() const
{
    return m_authority;
}

const std::string &Url::userinfo() const
{
    return m_userinfo;
}

const std::string &Url::host() const
{
    return m_host;
}

const std::string &Url::port() const
{
    return m_port;
}

const std::string &Url::path() const
{
    return m_path;
}

const std::map<std::string, std::string> &Url::query() const
{
    return m_query;
}

std::pair<std::string, bool> Url::query(const std::string &key) const
{
    auto it = m_query.find(key);
    if (it == m_query.end()) {
        return std::make_pair("", false);
    } else {
        return std::make_pair(it->second, true);
    }
}

const std::string &Url::fragment() const
{
    return m_fragment;
}

Url::Url()
{}

std::ostream &operator<<(std::ostream &os, const Url &url)
{
    os << url.str();
    return os;
}

std::string shortenUrl(const Url &url, size_t length = 20)
{
    return url.str();
}

} // namespace vistle
