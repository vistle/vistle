#ifndef VISTLE_UTIL_IPADDRESS_H
#define VISTLE_UTIL_IPADDRESS_H

#include <vector>
#include <boost/asio/ip/address.hpp>

#include "export.h"

namespace vistle {
std::vector<boost::asio::ip::address> V_UTILEXPORT getLocalAddresses();
}
#endif
