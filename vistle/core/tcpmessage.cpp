#include <boost/asio.hpp>

#include <util/tools.h>
#include "tcpmessage.h"
#include "message.h"

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {
namespace message {

bool recv(socket_t &sock, Message &msg, bool &received, bool block) {

   uint32_t sz = 0;

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

   uint32_t sz = 0;
   auto szbuf = asio::buffer(&sz, sizeof(sz));
   asio::async_read(sock, szbuf, [sz, &msg, &sock, handler](error_code ec, size_t n) {
       if (ec) {
           std::cerr << "message::async_recv err 1: ec=" << ec.message() << std::endl;
           handler(ec);
           return;
       }
       if (sz != n) {
           std::cerr << "message::async_recv: expected " << sz << ", received " << n << std::endl;
           handler(ec);
           return;
       }
       auto msgbuf = asio::buffer(&msg, sz);
       asio::async_read(sock, msgbuf, [&sock, handler](error_code ec, size_t n){
          std::cerr << "message::async_recv maybe err 2" << std::endl;
          handler(ec);
       });
   });
}

bool send(socket_t &sock, const Message &msg) {

   try {
      const uint32_t sz = htonl(msg.size());
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

      const uint32_t sz = htonl(msg.size());
      auto szbuf = asio::buffer(&sz, sizeof(sz));
      asio::async_write(sock, szbuf, [&msg, &sock, handler](error_code ec, size_t n){
         if (ec) {
            handler(ec);
            return;
         }
         auto msgbuf = asio::buffer(&msg, msg.size());
         asio::async_write(sock, msgbuf, [&sock, handler](error_code ec, size_t n){
            handler(ec);
         });
      });
}

} // namespace message
} // namespace vistle
