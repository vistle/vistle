#include "listenv4v6.h"

#include <boost/asio/ip/v6_only.hpp>

#ifndef WIN32 // does not work on windows the same way as on linux
#define REUSEADDR
#endif

namespace vistle {

bool start_listen(unsigned short port, boost::asio::ip::tcp::acceptor &acceptor_v4,
                  boost::asio::ip::tcp::acceptor &acceptor_v6, boost::system::error_code &ec)
{
    using namespace boost::asio;

    boost::system::error_code lec;
    ec = lec;

#ifdef REUSEADDR
    boost::asio::socket_base::reuse_address option(true);
#endif

    acceptor_v4.open(ip::tcp::v4(), lec);
#ifdef REUSEADDR
    acceptor_v4.set_option(option);
#endif
    if (lec == boost::system::errc::address_family_not_supported) {
    } else if (lec) {
        acceptor_v4.close();
        ec = lec;
        return false;
    } else {
        acceptor_v4.bind(ip::tcp::endpoint(ip::tcp::v4(), port), lec);
        if (lec) {
            acceptor_v4.close();
            ec = lec;
            return false;
        }
        acceptor_v4.listen(boost::asio::socket_base::max_listen_connections, lec);
        if (lec) {
            acceptor_v4.close();
            ec = lec;
            return false;
        }
    }

    acceptor_v6.open(ip::tcp::v6(), lec);
#ifdef REUSEADDR
    acceptor_v6.set_option(option);
#endif
    if (lec == boost::system::errc::address_family_not_supported) {
    } else if (lec) {
        acceptor_v4.close();
        acceptor_v6.close();
        ec = lec;
        return false;
    } else {
        acceptor_v6.set_option(ip::v6_only(true));
        acceptor_v6.bind(ip::tcp::endpoint(ip::tcp::v6(), port), lec);
        if (lec) {
            acceptor_v4.close();
            acceptor_v6.close();
            ec = lec;
            return false;
        }

        acceptor_v6.listen(boost::asio::socket_base::max_listen_connections, lec);
        if (lec) {
            acceptor_v4.close();
            acceptor_v6.close();
            ec = lec;
            return false;
        }
    }
#ifdef BOOST_POSIX_API
    if (acceptor_v4.is_open())
        fcntl(acceptor_v4.native_handle(), F_SETFD, FD_CLOEXEC);
    if (acceptor_v6.is_open())
        fcntl(acceptor_v6.native_handle(), F_SETFD, FD_CLOEXEC);
#endif

    return acceptor_v4.is_open() || acceptor_v6.is_open();
}

} // namespace vistle
