#include "vistleurl.h"
#include <vistle/util/url.h>

namespace vistle {

static const std::string scheme("vistle");

std::string VistleUrl::create(std::string host, unsigned short port, std::string key)
{
    ConnectionData data;
    data.host = host;
    data.port = port;
    data.hex_key = key;

    return create(data);
}

std::string VistleUrl::create(const ConnectionData &data)
{
    std::string url = scheme + "://" + data.host;
    if (data.port != 0) {
        url += ":" + std::to_string(data.port);
    }
    if (!data.kind.empty()) {
        url += "/" + data.kind;
    }
    if (!data.hex_key.empty()) {
        url += "?key=" + data.hex_key;
    }
    return url;
}

bool VistleUrl::parse(std::string urlstring, ConnectionData &data)
{
    Url url(urlstring);
    if (!url.valid() || url.scheme() != scheme)
        return false;

    try {
        if (url.authority().empty()) {
            data.master = true;

            auto port_have = url.query("port");
            if (port_have.second && !port_have.first.empty()) {
                data.port = std::stoi(port_have.first);
            }
        } else {
            data.host = url.host();
            if (data.host.empty())
                data.host = "localhost";
            if (!url.port().empty())
                data.port = std::stoi(url.port());
        }

        data.kind = url.path();
        auto key_have = url.query("key");
        if (key_have.second) {
            data.hex_key = key_have.first;
        }

    } catch (...) {
        return false;
    }

    return true;
}

} // namespace vistle
