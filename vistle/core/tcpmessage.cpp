#include <boost/asio.hpp>

#include <util/tools.h>
#include <arpa/inet.h>
#include "tcpmessage.h"
#include "message.h"

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {
namespace message {

typedef uint32_t SizeType;

bool check(const Message &msg) {
    if (msg.type() <= Message::ANY || msg.type() >= Message::NumMessageTypes) {
        std::cerr << "check message: invalid type " << msg.type() << std::endl;
        return false;
    }

    if (msg.size() <= 0 || msg.size() > Message::MESSAGE_SIZE) {
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

bool recv(socket_t &sock, Message &msg, bool &received, bool block) {

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
      auto szbuf = boost::asio::buffer(&sz, sizeof(sz));
      boost::system::error_code ec;
      asio::read(sock, szbuf, ec);
      if (ec) {
          std::cerr << "message::recv: size error " << ec.message() << std::endl;
          result = false;
          received = false;
      } else {
          sz = ntohl(sz);
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
          } else {
              assert(check(msg));
          }
      }
   } catch (std::exception &ex) {
      std::cerr << "message::recv: exception: " << ex.what() << std::endl;
      received = false;
      result = false;
   }

   return result;
}

void async_recv(socket_t &sock, message::Buffer &msg, std::function<void(boost::system::error_code ec)> handler) {

   struct RecvData {
       SizeType sz;
       asio::mutable_buffers_1 buf;
       message::Buffer &msg;
       RecvData(message::Buffer &msg)
           : buf(&sz, sizeof(sz))
           , msg(msg)
       {}
   };
   boost::shared_ptr<RecvData> recvData(new RecvData(msg));
   asio::async_read(sock, recvData->buf, [recvData, &msg, &sock, handler](error_code ec, size_t n) {
       if (ec) {
           std::cerr << "message::async_recv err 1: ec=" << ec.message() << std::endl;
           handler(ec);
           return;
       }
       if (n != sizeof(SizeType)) {
          std::cerr << "message::async_recv: expected " << sizeof(SizeType) << ", received " << n << std::endl;
          handler(ec);
          return;
       }
       recvData->sz = ntohl(recvData->sz);
       if (recvData->sz > msg.size()) {
           std::cerr << "message::async_recv err: buffer too small: have " << msg.size() << ", need " << recvData->sz << std::endl;
           abort();
           handler(error_code());
           return;
       }
       recvData->buf = asio::buffer(&recvData->msg, recvData->sz);
       asio::async_read(sock, recvData->buf, [recvData, &sock, handler](error_code ec, size_t n){
          if (recvData->sz != n) {
             std::cerr << "message::async_recv: expected " << recvData->sz << ", received " << n << std::endl;
             handler(ec);
             return;
          }
          if (ec) {
              std::cerr << "message::async_recv err 2: ec=" << ec.message() << std::endl;
          } else {
              assert(check(recvData->msg));
          }
          handler(ec);
       });
   });
}

bool send(socket_t &sock, const Message &msg, const std::vector<char> *payload) {

   assert(check(msg));
   try {
      const SizeType sz = htonl(msg.size());
      std::vector<boost::asio::const_buffer> buffers;
      buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
      buffers.push_back(boost::asio::buffer(&msg, msg.size()));
      if (payload)
          buffers.push_back(boost::asio::buffer(*payload));
      error_code ec;
      bool ret = asio::write(sock, buffers, ec);
      if (ec) {
          std::cerr << "message::send: error " << ec.message() << std::endl;
          ret = false;
      }
      return ret;
   } catch (std::exception &ex) {
      std::cerr << "message::send: exception: " << ex.what() << std::endl;
      return false;
   }
}

void async_send(socket_t &sock, const message::Message &msg, const std::function<void(error_code ec)> handler,
                boost::shared_ptr<std::vector<char>> payload) {

   assert(check(msg));
   std::cerr << "message::async_send: " << msg << std::endl;
   struct SendData {
      SizeType sz;
      const message::Buffer msg;
      boost::shared_ptr<std::vector<char>> payload;
      std::vector<boost::asio::const_buffer> buffers;
      SendData(const message::Message &msg, boost::shared_ptr<std::vector<char>> payload)
         : sz(htonl(msg.size()))
         , msg(msg)
         , payload(payload)
      {
          buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
          buffers.push_back(boost::asio::buffer(&msg, msg.size()));
          if (payload)
              buffers.push_back(boost::asio::buffer(*payload));
      }
   };
   boost::shared_ptr<SendData> sendData(new SendData(msg, payload));

   asio::async_write(sock, sendData->buffers, [sendData, &sock, handler](error_code ec, size_t n){
      if (ec) {
         std::cerr << "message::async_send error: ec=" << ec.message() << std::endl;
      }
      handler(ec);
   });
}

} // namespace message
} // namespace vistle
