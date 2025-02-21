#include "ipaddress.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include <boost/asio.hpp>

namespace vistle {

std::vector<boost::asio::ip::address> getLocalAddresses()
{
    std::vector<boost::asio::ip::address> addresses;

    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "getLocalAddresses: error in getifaddrs: " << strerror(errno) << std::endl;
        return addresses;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6)
            continue;

        static const auto localhost4 = boost::asio::ip::make_address_v4("127.0.0.1");
        static const auto localhost6 = boost::asio::ip::make_address_v6("::1");

        // Try to get numeric address
        char host[NI_MAXHOST];
        if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
            //std::cerr << "ip address: " << host << std::endl;

            switch (family) {
            case AF_INET: {
                auto addr = boost::asio::ip::make_address_v4(host);
                if (addr == localhost4)
                    continue;
                addresses.push_back(addr);
                break;
            }
            case AF_INET6: {
                // skip link-local addresses
                if (strchr(host, '%')) {
                    continue;
                }
                auto addr = boost::asio::ip::make_address_v6(host);
                if (addr == localhost6)
                    continue;
                addresses.push_back(addr);
                break;
            }
            }
        }
    }

    freeifaddrs(ifaddr);

    return addresses;
}

} // namespace vistle
