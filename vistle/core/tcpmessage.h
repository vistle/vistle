#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#include <boost/asio/ip/tcp.hpp>

#include "export.h"

namespace vistle {

namespace message {

class Message;
class Buffer;

typedef boost::asio::ip::tcp::socket socket_t;
using boost::system::error_code;

bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, const std::vector<char> *payload=nullptr);
bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, error_code &ec, const std::vector<char> *payload=nullptr);
void V_COREEXPORT async_send(socket_t &sock, const Message &msg,
                             std::shared_ptr<std::vector<char>> payload,
                             const std::function<void(error_code ec)> handler);
void V_COREEXPORT async_forward(socket_t &sock, const Message &msg,
                             std::shared_ptr<socket_t> payloadSock,
                             const std::function<void(error_code ec)> handler);

bool V_COREEXPORT recv(socket_t &sock, message::Buffer &msg, error_code &ec, bool block=false, std::vector<char> *payload=nullptr);
bool V_COREEXPORT recv_message(socket_t &sock, message::Buffer &msg, error_code &ec, bool block=false);
void V_COREEXPORT async_recv(socket_t &sock, vistle::message::Buffer &msg, std::function<void(error_code, std::shared_ptr<std::vector<char>>)> handler);
void V_COREEXPORT async_recv_header(socket_t &sock, vistle::message::Buffer &msg, std::function<void(error_code)> handler);

void V_COREEXPORT return_buffer(std::shared_ptr<std::vector<char>> &buf);
std::shared_ptr<std::vector<char>> V_COREEXPORT get_buffer(size_t size = 0);

} // namespace message
} // namespace vistle
#endif
