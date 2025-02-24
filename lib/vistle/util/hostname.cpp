#include "hostname.h"

#include <cstdlib>
#include <boost/asio/ip/host_name.hpp>
#include <iostream>

namespace vistle {

static std::string cluster;

std::string clustername()
{
    if (cluster.empty()) {
        const char *cn = getenv("VISTLE_CLUSTER");
        if (cn) {
            cluster = cn;
        }
    }
    return cluster;
}

std::string hostname()
{
    static std::string hname;
    if (!hname.empty()) {
        return hname;
    }

    const char *hn = getenv("VISTLE_HOSTNAME");
    if (hn) {
        hname = hn;
    } else {
        boost::system::error_code ec;
        hname = boost::asio::ip::host_name(ec);
        if (ec) {
            std::cerr << "failed to get hostname: " << ec.message() << std::endl;
        }
    }

    return hname;
}

} // namespace vistle
