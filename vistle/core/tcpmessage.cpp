#include <boost/asio.hpp>

#include "tcpmessage.h"
#include "message.h"

namespace asio = boost::asio;

namespace vistle {
namespace message {

bool recv(socket_t &sock, Message &msg, bool &received) {

   uint32_t sz = 0;

   boost::asio::socket_base::bytes_readable command(true);
   sock.io_control(command);
   std::size_t bytes_readable = command.get();
   if (bytes_readable < sizeof(sz)) {
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
      asio::read(sock, msgbuf);
   } catch (std::exception &ex) {
      std::cerr << "message::recv: exception: " << ex.what() << std::endl;
      received = false;
      result = false;
   }

   return result;
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

} // namespace message
} // namespace vistle
