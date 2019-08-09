#include <iostream>
#include <functional>
#include <memory>

#include <boost/asio.hpp>
#include <mutex>

#include <util/tools.h>
#include <util/buffer.h>
#include "tcpmessage.h"
#include "message.h"
#include "messages.h"
#include <deque>

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {
namespace message {

typedef uint32_t SizeType;

static const size_t buffersize = 16384;

//#define USE_BUFFER_POOL

namespace {

bool check(const Message &msg, const char *payload, size_t size) {
    if (msg.type() <= ANY || msg.type() >= NumMessageTypes) {
        std::cerr << "check message: invalid type " << msg.type() << std::endl;
        return false;
    }

    if (msg.size() < sizeof(Message) || msg.size() > Message::MESSAGE_SIZE) {
        std::cerr << "check message: invalid size " << msg.size() << std::endl;
        return false;
    }

    if (payload && msg.payloadSize() > 0 && size != msg.payloadSize()) {
        std::cerr << "check message: payload and size do not match" << std::endl;
        return false;
    }

    return true;
}

bool check(const Message &msg, const buffer *payload) {

    return check(msg, payload?payload->data():nullptr, payload?payload->size():0);
}

}

#ifdef USE_BUFFER_POOL
namespace  {

std::mutex buffer_pool_mutex;
std::deque<std::shared_ptr<buffer>> buffer_pool;

}
#endif


void return_buffer(std::shared_ptr<buffer> &buf) {
#ifdef USE_BUFFER_POOL
    if (!buf) {
        //std::cerr << "empty buffer returned" << std::endl;
        return;
    }

    //std::cerr << "buffer returned, capacity=" << buf->capacity() << std::endl;
    std::lock_guard<std::mutex> lock(buffer_pool_mutex);
    buffer_pool.emplace_back(buf);
#else
    (void)buf;
#endif
}

std::shared_ptr<buffer> get_buffer(size_t size) {
#ifdef USE_BUFFER_POOL
    {
        std::lock_guard<std::mutex> lock(buffer_pool_mutex);
        if (!buffer_pool.empty()) {
            auto best = buffer_pool.front();
            buffer_pool.pop_front();
            for (auto &buf: buffer_pool) {
                if (buf->size() >= size && buf->size() < best->size()) {
                    std::swap(buf, best);
                } else if (best->size() < size && buf->size() > best->size()) {
                    std::swap(buf, best);
                }
            }
            //std::cerr << "buffer reuse, requested=" << size << ", capacity=" << best->capacity() << std::endl;
            return best;
        }
    }

    //std::cerr << "new buffer" << std::endl;
#endif
    return std::make_shared<buffer>();
}

namespace {

struct SendRequest;
std::mutex sendQueueMutex;
std::map<socket_t*, std::deque<std::shared_ptr<SendRequest>>> sendQueues;
void submitSendRequest(std::shared_ptr<SendRequest> req);

struct SendRequest {
    socket_t &sock;
    const message::Buffer msg;
    std::shared_ptr<buffer> payload;
    std::shared_ptr<socket_t> payloadSocket;
    std::function<void(error_code)> handler;
    SendRequest(socket_t &sock, const message::Message &msg, std::shared_ptr<buffer> payload, std::function<void(error_code)> handler)
        : sock(sock)
        , msg(msg)
        , payload(payload)
        , handler(handler)
    {
    }

    SendRequest(socket_t &sock, const message::Message &msg, std::shared_ptr<socket_t> payloadSocket, std::function<void(error_code)> handler)
        : sock(sock)
        , msg(msg)
        , payloadSocket(payloadSocket)
        , handler(handler)
    {
    }

    void operator()() {

        error_code ec;
        size_t n = msg.payloadSize();
        if (payload) {
            if (n >= payload->size()) {
                n -= payload->size();
            } else {
                n = 0;
            }
        }
        if (send(sock, msg, ec, payload.get()) && payloadSocket) {
            buffer bufvec(buffersize);
            for (size_t i = 0; i < n;) {
                auto buf = asio::buffer(bufvec.data(), std::min(bufvec.size(), n-i));
                ssize_t c = asio::read(*payloadSocket, buf, ec);
                if (ec) {
                    break;
                }
                if (c == 0) {
                    ec = boost::asio::error::make_error_code(boost::asio::error::misc_errors::eof);
                    break;
                }
                auto buf2 = asio::buffer(bufvec.data(), c);
                ssize_t w = asio::write(sock, buf2, ec);
                if (w != c) {
                    break;
                }
                i += c;
            }
        }
        handler(ec);

        std::lock_guard<std::mutex> locker(sendQueueMutex);
        auto &sq = sendQueues[&sock];
        assert(!sq.empty());
        auto This = sq.front();
        sq.pop_front();

        if (!sq.empty()) {
            auto &req = sq.front();
            submitSendRequest(req);
        }
    }
};


void submitSendRequest(std::shared_ptr<SendRequest> req) {
    //std::cerr << "submitSendRequest: " << sendQueues[&req->sock].size() << " requests queued for " << &req->sock << std::endl;
#if BOOST_VERSION >= 106900
    req->sock.get_executor().post(*req, std::allocator<char>());
#else
	req->sock.get_io_service().post(*req);
#endif
}

}

bool recv_payload(socket_t &sock, message::Buffer &msg, error_code &ec, buffer *payload) {

    if (msg.payloadSize() > 0) {
        buffer pl;
        if (!payload) {
            std::cerr << "message::recv: ignoring payload: " << msg << std::endl;
            payload = &pl;
        }
        payload->resize(msg.payloadSize());
        auto buf = asio::buffer(payload->data(), payload->size());
        size_t sz = asio::read(sock, buf, ec);
        if (ec) {
            std::cerr << "message::recv: payload error " << ec.message() << std::endl;
            return false;
        } else if (sz != msg.payloadSize()) {
            std::cerr << "message::recv: short payload read: received " << sz << " instead of " << payload->size() << std::endl;
            return false;
        }
    }

    return true;
}

namespace {

struct RecvRequest;
std::mutex recvQueueMutex;
std::map<socket_t *, std::deque<std::shared_ptr<RecvRequest>>> recvQueues;
void submitRecvRequest(std::shared_ptr<RecvRequest> req);

struct RecvRequest {

    socket_t &sock;
    message::Buffer &msg;
    std::function<void(error_code, std::shared_ptr<buffer>)> handler;
    std::function<void(error_code, message::Buffer &msg)> payload_handler;
    bool no_payload = false;

    RecvRequest(socket_t &sock, message::Buffer &msg, std::function<void(error_code, std::shared_ptr<buffer>)> handler)
        : sock(sock)
        , msg(msg)
        , handler(handler)
    {
    }

    void operator()() {
        error_code ec;
        bool error = false;
        if (!recv_message(sock, msg, ec, true)) {
            error = true;
            handler(ec, std::shared_ptr<buffer>());
        }
        std::shared_ptr<buffer> payload;
        if (!no_payload && !error && msg.payloadSize() > 0) {
            payload = get_buffer(msg.payloadSize());
            if (!recv_payload(sock, msg, ec, payload.get())) {
                error = true;
                handler(ec, std::shared_ptr<buffer>());
            }
        }
        if (!error) {
            handler(ec, payload);
        }

        std::lock_guard<std::mutex> locker(recvQueueMutex);
        auto &rq = recvQueues[&sock];
        assert(!rq.empty());
        auto This = rq.front();
        rq.pop_front();
        if (!rq.empty()) {
            auto &req = rq.front();
            submitRecvRequest(req);
        }
    }

};

void submitRecvRequest(std::shared_ptr<RecvRequest> req) {
    //std::cerr << "submitRecvRequest: " << recvQueues[&req->sock].size() << " requests queued for " << &req->sock << std::endl;
#if BOOST_VERSION >= 106900
    req->sock.get_executor().post(*req, std::allocator<char>());
#else
	req->sock.get_io_service().post(*req);
#endif
}

}

bool recv_message(socket_t &sock, message::Buffer &msg, error_code &ec, bool block) {

   if (!sock.is_open()) {
       ec = boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor);
       return false;
   }

   char emptybuf[1];
   auto empty = boost::asio::buffer(&emptybuf, 0);
   asio::read(sock, empty, ec);
   if (ec) {
       std::cerr << "message::recv: error " << ec.message() << std::endl;
       return false;
   }

   SizeType sz = 0;

   boost::asio::socket_base::bytes_readable command(true);
   sock.io_control(command);
   std::size_t bytes_readable = command.get();
   if (bytes_readable < sizeof(sz) && !block) {
      return false;
   }

   auto szbuf = boost::asio::buffer(&sz, sizeof(sz));
   asio::read(sock, szbuf, ec);
   if (ec) {
       std::cerr << "message::recv: size error " << ec.message() << std::endl;
       return false;
   }

   sz = ntohl(sz);
   if (sz < sizeof(Message)) {
       std::cerr << "message::recv: msg size too small: " << sz << ", min is " << sizeof(Message) << std::endl;
   }
   assert(sz >= sizeof(Message));
   if (sz > Message::MESSAGE_SIZE) {
       std::cerr << "message::recv: msg size too large: " << sz << ", max is " << Message::MESSAGE_SIZE << std::endl;
   }
   assert(sz <= Message::MESSAGE_SIZE);
   auto msgbuf = asio::buffer(&msg, sz);
   asio::read(sock, msgbuf, ec);
   if (ec) {
       std::cerr << "message::recv: msg error " << ec.message() << std::endl;
       return false;
   }

   return true;
}

bool recv(socket_t &sock, message::Buffer &msg, error_code &ec, bool block, buffer *payload) {

    if (!recv_message(sock, msg, ec, block)) {
        return false;
    }

   if (!recv_payload(sock, msg, ec, payload)) {
       return false;
   }

   return true;
}

void async_recv(socket_t &sock, message::Buffer &msg, std::function<void(boost::system::error_code ec, std::shared_ptr<buffer>)> handler) {

   auto req = std::make_shared<RecvRequest>(sock, msg, handler);

   std::lock_guard<std::mutex> locker(recvQueueMutex);
   bool submit = recvQueues[&sock].empty();
   recvQueues[&sock].emplace_back(req);
   //std::cerr << "message::async_recv: " << recvQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitRecvRequest(req);
   }
}

void async_recv_header(socket_t &sock, message::Buffer &msg, std::function<void(boost::system::error_code ec)> handler) {

   auto h = [handler](boost::system::error_code ec, std::shared_ptr<buffer>){
       handler(ec);
   };
   auto req = std::make_shared<RecvRequest>(sock, msg, h);
   req->no_payload = true;

   std::lock_guard<std::mutex> locker(recvQueueMutex);
   bool submit = recvQueues[&sock].empty();
   recvQueues[&sock].emplace_back(req);
   //std::cerr << "message::async_recv: " << recvQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitRecvRequest(req);
   }
}

bool send(socket_t &sock, const Message &msg, const buffer *payload) {

    assert(check(msg, payload));

    const SizeType sz = htonl(msg.size());
    std::vector<boost::asio::const_buffer> buffers;
    //buffers.push_back(boost::asio::buffer(&InitialMark, sizeof(InitialMark)));
    buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
    buffers.push_back(boost::asio::buffer(&msg, msg.size()));
    if (payload && payload->size()) {
        buffers.push_back(boost::asio::buffer(*payload));
        //buffers.push_back(boost::asio::buffer(&EndPayloadMark, sizeof(EndPayloadMark)));
    }
    error_code ec;
    asio::write(sock, buffers, ec);
    if (ec) {
        std::cerr << "message::send: error: " << ec.message() << std::endl;
        if (ec != boost::system::errc::connection_reset)
            std::cerr << backtrace() << std::endl;
        return false;
    }
    return true;
}

bool send(socket_t &sock, const message::Message &msg, error_code &ec, const buffer *payload) {

   assert(check(msg, payload));

   const SizeType sz = htonl(msg.size());
   std::vector<boost::asio::const_buffer> buffers;
   //buffers.push_back(boost::asio::buffer(&InitialMark, sizeof(InitialMark)));
   buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
   buffers.push_back(boost::asio::buffer(&msg, msg.size()));
   if (payload && payload->size()) {
       buffers.push_back(boost::asio::buffer(*payload));
       //buffers.push_back(boost::asio::buffer(&EndPayloadMark, sizeof(EndPayloadMark)));
   }

   asio::write(sock, buffers, ec);
   if (ec) {
       std::cerr << "message::send: error: " << ec.message() << std::endl;
       if (ec != boost::system::errc::connection_reset)
           std::cerr << backtrace() << std::endl;
       return false;
   }

   return true;
}

bool send(socket_t &sock, const message::Message &msg, error_code &ec, const char *payload, size_t size) {

   const SizeType sz = htonl(msg.size());
   std::vector<boost::asio::const_buffer> buffers;
   //buffers.push_back(boost::asio::buffer(&InitialMark, sizeof(InitialMark)));
   buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
   buffers.push_back(boost::asio::buffer(&msg, msg.size()));
   if (payload && size>0) {
       buffers.push_back(boost::asio::buffer(payload, size));
       //buffers.push_back(boost::asio::buffer(&EndPayloadMark, sizeof(EndPayloadMark)));
   }

   asio::write(sock, buffers, ec);
   if (ec) {
       std::cerr << "message::send: error: " << ec.message() << std::endl;
       if (ec != boost::system::errc::connection_reset)
           std::cerr << backtrace() << std::endl;
       return false;
   }

   return true;
}


void async_send(socket_t &sock, const message::Message &msg,
                std::shared_ptr<buffer> payload,
                const std::function<void(error_code ec)> handler)
{
   assert(check(msg, payload.get()));
   auto req = std::make_shared<SendRequest>(sock, msg, payload, handler);

   std::lock_guard<std::mutex> locker(sendQueueMutex);
   bool submit = sendQueues[&sock].empty();
   sendQueues[&sock].emplace_back(req);
   //std::cerr << "message::async_send: " << sendQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitSendRequest(req);
   }
}

void async_forward(socket_t &sock, const message::Message &msg,
                std::shared_ptr<socket_t> payloadSock,
                const std::function<void(error_code ec)> handler)
{
   auto req = std::make_shared<SendRequest>(sock, msg, payloadSock, handler);

   std::lock_guard<std::mutex> locker(sendQueueMutex);
   bool submit = sendQueues[&sock].empty();
   sendQueues[&sock].emplace_back(req);
   //std::cerr << "message::async_forward: " << sendQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitSendRequest(req);
   }
}

} // namespace message
} // namespace vistle
