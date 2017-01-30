#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#include <boost/asio/ip/tcp.hpp>

#include "export.h"

namespace vistle {

namespace message {

class Message;
class Buffer;

typedef boost::asio::ip::tcp::socket socket_t;

bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, const std::vector<char> *payload=nullptr);
void V_COREEXPORT async_send(socket_t &sock, const Message &msg,
                             std::shared_ptr<std::vector<char>> payload,
                             const std::function<void(boost::system::error_code ec)> handler);

bool V_COREEXPORT recv(socket_t &sock, message::Buffer &msg, bool &received, bool block=false, std::vector<char> *payload=nullptr);
void V_COREEXPORT async_recv(socket_t &sock, vistle::message::Buffer &msg, std::function<void(boost::system::error_code, std::shared_ptr<std::vector<char>>)> handler);

} // namespace message
} // namespace vistle
#endif
