#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#include <boost/asio/ip/tcp.hpp>

#include <vistle/util/buffer.h>

#include "messagepayload.h"

#include "export.h"

namespace vistle {

namespace message {

class Message;
class Buffer;

typedef boost::asio::ip::tcp::socket socket_t;
using boost::system::error_code;

bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, const buffer *payload = nullptr);
bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, error_code &ec, const buffer *payload = nullptr);
bool V_COREEXPORT send(socket_t &sock, const message::Message &msg, error_code &ec, const char *payload, size_t size);
void V_COREEXPORT async_send(socket_t &sock, const Message &msg, std::shared_ptr<buffer> payload,
                             const std::function<void(error_code ec)> handler);
void V_COREEXPORT async_send(socket_t &sock, const Message &msg, const MessagePayload &payload,
                             const std::function<void(error_code ec)> handler);
void V_COREEXPORT async_forward(socket_t &sock, const Message &msg, std::shared_ptr<socket_t> payloadSock,
                                const std::function<void(error_code ec)> handler);

bool V_COREEXPORT recv(socket_t &sock, message::Buffer &msg, error_code &ec, bool block = false,
                       buffer *payload = nullptr);
bool V_COREEXPORT recv_message(socket_t &sock, message::Buffer &msg, error_code &ec, bool block = false);
void V_COREEXPORT async_recv(socket_t &sock, vistle::message::Buffer &msg,
                             std::function<void(error_code, std::shared_ptr<buffer>)> handler);
void V_COREEXPORT async_recv_header(socket_t &sock, vistle::message::Buffer &msg,
                                    std::function<void(error_code)> handler);

void V_COREEXPORT return_buffer(std::shared_ptr<buffer> &buf);
std::shared_ptr<buffer> V_COREEXPORT get_buffer(size_t size = 0);

bool V_COREEXPORT clear_request_queue();

} // namespace message
} // namespace vistle
#endif
