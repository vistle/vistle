#include "uiclient.h"
#include <core/tcpmessage.h>
#include <util/sysdep.h>
#include "uimanager.h"

namespace asio = boost::asio;

namespace vistle {

UiClient::UiClient(UiManager &manager, int id, boost::shared_ptr<asio::ip::tcp::socket> socket)
: m_id(id)
, m_done(false)
, m_socket(socket)
, m_manager(manager)
{
}

UiClient::~UiClient() {

}

int UiClient::id() const {

   return m_id;
}

UiManager &UiClient::manager() const {

   return m_manager;
}

bool UiClient::done() const {

   return m_done;
}


void UiClient::operator()() {

   while (!m_done) {

      bool received = false, sent = false;

      socket().get_io_service().poll();
      message::Buffer buf;
      if (!message::recv(socket(), buf.msg, received))
         break;

      if (received) {
         if (buf.msg.type() == message::Message::MODULEEXIT) {
            std::cerr << "user interface " << id() << " quit (sender: " << buf.msg.senderId() << ")" << std::endl;
            m_done = true;
         } else {
            while (!manager().handleMessage(buf.msg))
               usleep(100000);
         }
      }

      if (!received && !sent)
         usleep(10000);
   }

   m_done = true;
}

void UiClient::cancel() {

   m_done = true;
}

asio::ip::tcp::socket &UiClient::socket() {

   return *m_socket;
}

} // namespace vistle
