#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#include <boost/asio/ip/tcp.hpp>

#include "export.h"

namespace vistle {

namespace message {

class Message;

typedef boost::asio::ip::tcp::socket socket_t;

bool send(socket_t &sock, const message::Message &msg);
bool recv(socket_t &sock, message::Message &msg, bool &received);

} // namespace message
} // namespace vistle
#endif
