#include "ipaddress.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
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

    static const auto localhost4 = boost::asio::ip::make_address_v4("127.0.0.1");
    static const auto localhost6 = boost::asio::ip::make_address_v6("::1");

#ifdef _WIN32
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    ULONG family = AF_UNSPEC;
    ULONG bufferSize = 0;

    // First call to get required buffer size
    if (GetAdaptersAddresses(family, flags, nullptr, nullptr, &bufferSize) != ERROR_BUFFER_OVERFLOW) {
        return addresses;
    }

    std::vector<char> buffer(bufferSize);
    IP_ADAPTER_ADDRESSES *adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());

    if (GetAdaptersAddresses(family, flags, nullptr, adapters, &bufferSize) != NO_ERROR) {
        return addresses;
    }

    for (auto *adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
        for (auto *ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
            auto *sa = ua->Address.lpSockaddr;
            if (!sa)
                continue;

            char host[NI_MAXHOST] = {};

            if (getnameinfo(sa, (sa->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6), host,
                            NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) != 0) {
                continue;
            }

            try {
                if (sa->sa_family == AF_INET) {
                    auto addr = boost::asio::ip::make_address_v4(host);
                    if (addr == localhost4)
                        continue;
                    addresses.push_back(addr);
                } else if (sa->sa_family == AF_INET6) {
                    // Skip link-local (fe80::/10) and scoped addresses
                    if (strchr(host, '%'))
                        continue;

                    auto addr = boost::asio::ip::make_address_v6(host);
                    if (addr == localhost6)
                        continue;
                    addresses.push_back(addr);
                }
            } catch (...) {
                // ignore malformed addresses
            }
        }
    }
#else
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
#endif

    return addresses;
}

} // namespace vistle
