#ifndef VISTLE_UTIL_LISTENV4V6_H
#define VISTLE_UTIL_LISTENV4V6_H


#include <boost/asio/ip/tcp.hpp>
#include "export.h"

namespace vistle {

bool V_UTILEXPORT start_listen(unsigned short port, boost::asio::ip::tcp::acceptor &acceptor_v4,
                               boost::asio::ip::tcp::acceptor &acceptor_v6, boost::system::error_code &ec);

}
#endif
