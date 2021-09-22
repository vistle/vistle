#include "hostname.h"

#include <cstdlib>
#include <boost/asio/ip/host_name.hpp>
#include <iostream>

namespace vistle {

std::string hostname()
{
    static std::string hname;

    if (hname.empty()) {
#if 0
      const size_t HOSTNAMESIZE = 256;

      char hostname[HOSTNAMESIZE];
      gethostname(hostname, HOSTNAMESIZE-1);
      hostname[HOSTNAMESIZE-1] = '\0';
      hname = hostname;
#else
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
#endif
    }
    return hname;
}

} // namespace vistle
