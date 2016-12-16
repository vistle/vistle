#include <iostream>
#include <functional>

#include <boost/asio.hpp>
#include <mutex>

#include <util/tools.h>
#include <arpa/inet.h>
#include "tcpmessage.h"
#include "message.h"
#include "messages.h"

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {
namespace message {

typedef uint32_t SizeType;
static const uint32_t VistleError = 12345;

namespace {

bool check(const Message &msg) {
    if (msg.type() <= Message::ANY || msg.type() >= Message::NumMessageTypes) {
        std::cerr << "check message: invalid type " << msg.type() << std::endl;
        return false;
    }

    if (msg.size() < sizeof(Message) || msg.size() > Message::MESSAGE_SIZE) {
        std::cerr << "check message: invalid size " << msg.size() << std::endl;
        return false;
    }

    if (msg.rank() < -1) {
        std::cerr << "check message: invalid source rank " << msg.rank() << std::endl;
        return false;
    }

    if (msg.destRank() < -1) {
        std::cerr << "check message: invalid destination rank " << msg.destRank() << std::endl;
        return false;
    }

    return true;
}

}

namespace {

struct SendRequest;
std::mutex sendQueueMutex;
std::map<socket_t*, std::deque<boost::shared_ptr<SendRequest>>> sendQueues;
void submitSendRequest(boost::shared_ptr<SendRequest> req);

struct SendRequest {
    socket_t &sock;
    const message::Buffer msg;
    boost::shared_ptr<std::vector<char>> payload;
    std::function<void(error_code)> handler;
    SendRequest(socket_t &sock, const message::Message &msg, boost::shared_ptr<std::vector<char>> payload, std::function<void(error_code)> handler)
        : sock(sock)
        , msg(msg)
        , payload(payload)
        , handler(handler)
    {
    }

    void operator()() {

        error_code ec;
        if (!send(sock, msg, payload.get())) {
            ec.assign(VistleError, boost::system::generic_category());
        }
        handler(ec);

        std::lock_guard<std::mutex> locker(sendQueueMutex);
        assert(!sendQueues[&sock].empty());
        auto This = sendQueues[&sock].front();
        sendQueues[&sock].pop_front();

        if (!sendQueues[&sock].empty()) {
            auto &req = sendQueues[&sock].front();
            submitSendRequest(req);
        }
    }
};


void submitSendRequest(boost::shared_ptr<SendRequest> req) {
    //std::cerr << "submitSendRequest: " << sendQueues[&req->sock].size() << " requests queued for " << &req->sock << std::endl;
    req->sock.get_io_service().post(*req);
}

}

namespace {

struct RecvRequest;
std::mutex recvQueueMutex;
std::map<socket_t *, std::deque<boost::shared_ptr<RecvRequest>>> recvQueues;
void submitRecvRequest(boost::shared_ptr<RecvRequest> req);

struct RecvRequest {

    socket_t &sock;
    message::Buffer &msg;
    std::function<void(error_code, boost::shared_ptr<std::vector<char>>)> handler;

    RecvRequest(socket_t &sock, message::Buffer &msg, std::function<void(error_code, boost::shared_ptr<std::vector<char>>)> handler)
        : sock(sock)
        , msg(msg)
        , handler(handler)
    {
    }

    void operator()() {
        bool received = true;
        error_code ec;
        boost::shared_ptr<std::vector<char>> payload(new std::vector<char>);
        if (!recv(sock, msg, received, true, payload.get())) {
            ec.assign(VistleError, boost::system::generic_category());
        }
        handler(ec, payload);

        std::lock_guard<std::mutex> locker(recvQueueMutex);
        assert(!recvQueues[&sock].empty());
        auto This = recvQueues[&sock].front();
        recvQueues[&sock].pop_front();
        if (!recvQueues[&sock].empty()) {
            auto &req = recvQueues[&sock].front();
            submitRecvRequest(req);
        }
    }

};

void submitRecvRequest(boost::shared_ptr<RecvRequest> req) {
    //std::cerr << "submitRecvRequest: " << recvQueues[&req->sock].size() << " requests queued for " << &req->sock << std::endl;
    req->sock.get_io_service().post(*req);
}

}

bool recv(socket_t &sock, message::Buffer &msg, bool &received, bool block, std::vector<char> *payload) {

   SizeType sz = 0;

   boost::asio::socket_base::bytes_readable command(true);
   sock.io_control(command);
   std::size_t bytes_readable = command.get();
   if (bytes_readable < sizeof(sz) && !block) {
      received = false;
      return true;
   }

   received = true;

   bool result = true;

   try {
       boost::system::error_code ec;
       auto szbuf = boost::asio::buffer(&sz, sizeof(sz));
       asio::read(sock, szbuf, ec);
       if (ec) {
           std::cerr << "message::recv: size error " << ec.message() << std::endl;
           result = false;
           received = false;
       } else {
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
               result = false;
               received = false;
           } else if (msg.payloadSize() > 0) {
               std::vector<char> pl;
               if (!payload)
                   payload = &pl;
               payload->resize(msg.payloadSize());
               auto buf = asio::buffer(payload->data(), payload->size());
               asio::read(sock, buf, ec);
               if (ec) {
                   std::cerr << "message::recv: payload error " << ec.message() << std::endl;
                   result = false;
                   received = false;
               } else {
                   //std::cerr << "message::recv: payload of size " << payload->size() << " received" << std::endl;
               }
           }
       }
   } catch (std::exception &ex) {
      std::cerr << "message::recv: exception: " << ex.what() << std::endl;
      received = false;
      result = false;
   }

   return result;
}

void async_recv(socket_t &sock, message::Buffer &msg, std::function<void(boost::system::error_code ec, boost::shared_ptr<std::vector<char>>)> handler) {

   boost::shared_ptr<RecvRequest> req(new RecvRequest(sock, msg, handler));

   std::lock_guard<std::mutex> locker(recvQueueMutex);
   bool submit = recvQueues[&sock].empty();
   recvQueues[&sock].push_back(req);
   //std::cerr << "message::async_recv: " << recvQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitRecvRequest(req);
   }
}

bool send(socket_t &sock, const Message &msg, const std::vector<char> *payload) {

   assert(check(msg));
   try {
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
      bool ret = asio::write(sock, buffers, ec);
      if (ec) {
          std::cerr << "message::send: error " << ec.message() << std::endl;
          std::cerr << backtrace() << std::endl;
          ret = false;
      }
      return ret;
   } catch (std::exception &ex) {
      std::cerr << "message::send: exception: " << ex.what() << std::endl;
      std::cerr << backtrace() << std::endl;
      return false;
   }
}

void async_send(socket_t &sock, const message::Message &msg,
                boost::shared_ptr<std::vector<char>> payload,
                const std::function<void(error_code ec)> handler)
{
   assert(check(msg));
   boost::shared_ptr<SendRequest> req(new SendRequest(sock, msg, payload, handler));

   std::lock_guard<std::mutex> locker(sendQueueMutex);
   bool submit = sendQueues[&sock].empty();
   sendQueues[&sock].push_back(req);
   //std::cerr << "message::async_send: " << sendQueues[&sock].size() << " requests queued for " << &sock << std::endl;

   if (submit) {
       submitSendRequest(req);
   }
}

} // namespace message
} // namespace vistle
