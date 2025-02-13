#ifndef VISTLE_CONTROL_VISTLEURL_H
#define VISTLE_CONTROL_VISTLEURL_H

#include "export.h"
#include <string>

namespace vistle {

class V_HUBEXPORT ConnectionData {
public:
    bool master = false;
    std::string host;
    unsigned short port = 0;
    std::string hex_key;
    std::string kind;
    std::string conference_url;
};


class V_HUBEXPORT VistleUrl {
public:
    static bool parse(std::string url, ConnectionData &data);
    static std::string create(std::string host, unsigned short port, std::string key);
    static std::string create(const ConnectionData &data);
};

} // namespace vistle
#endif
