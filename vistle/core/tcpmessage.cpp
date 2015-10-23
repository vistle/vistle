#include <boost/asio.hpp>

#include <util/tools.h>
#include "tcpmessage.h"
#include "message.h"

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {
namespace message {

typedef uint32_t SizeType;

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
      boost::system::error_code error;
      asio::read(sock, szbuf);
      sz = ntohl(sz);
      auto msgbuf = asio::buffer(&msg, sz);
      assert(sz <= Message::MESSAGE_SIZE);
      asio::read(sock, msgbuf);
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
       recvData->buf = asio::buffer(&recvData->msg, recvData->sz);
       asio::async_read(sock, recvData->buf, [recvData, &sock, handler](error_code ec, size_t n){
          if (recvData->sz != n) {
             std::cerr << "message::async_recv: expected " << recvData->sz << ", received " << n << std::endl;
             handler(ec);
             return;
          }
          if (ec) {
              std::cerr << "message::async_recv err 2: ec=" << ec.message() << std::endl;
          }
          handler(ec);
       });
   });
}

bool send(socket_t &sock, const Message &msg) {

   try {
      const SizeType sz = htonl(msg.size());
      auto szbuf = asio::buffer(&sz, sizeof(sz));
      asio::write(sock, szbuf);
      auto msgbuf = asio::buffer(&msg, msg.size());
      return asio::write(sock, msgbuf);
   } catch (std::exception &ex) {
      std::cerr << "message::send: exception: " << ex.what() << std::endl;
      return false;
   }
}

void async_send(socket_t &sock, const message::Message &msg, const std::function<void(error_code ec)> handler) {

   std::cerr << "message::async_send: " << msg << std::endl;
   struct SendData {
      SizeType sz;
      const message::Buffer msg;
      SendData(const message::Message &msg)
         : sz(htonl(msg.size()))
         , msg(msg)
      {}
   };
   boost::shared_ptr<SendData> sendData(new SendData(msg));

   asio::async_write(sock, asio::buffer(&sendData->sz, sizeof(sendData->sz)), [sendData, &sock, handler](error_code ec, size_t n){
      if (ec) {
         std::cerr << "message::async_send err 1: ec=" << ec.message() << std::endl;
         handler(ec);
         return;
      }
      asio::async_write(sock, asio::buffer(&sendData->msg, ntohl(sendData->sz)), [sendData, &sock, handler](error_code ec, size_t n){
          if (ec) {
             std::cerr << "message::async_send err 2: ec=" << ec.message() << std::endl;
          }
         handler(ec);
      });
   });
}

} // namespace message
} // namespace vistle
