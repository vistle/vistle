#include "uiclient.h"
#include <core/tcpmessage.h>
#include <util/sysdep.h>

namespace asio = boost::asio;

namespace vistle {

UiClient::UiClient(UiManager &manager, int id, boost::shared_ptr<message::MessageQueue> sendQueue, boost::shared_ptr<message::MessageQueue> recvQueue)
: m_id(id)
, m_done(false)
, m_socket(new asio::ip::tcp::socket(m_ioService))
, m_manager(manager)
, m_sendQueue(sendQueue)
, m_recvQueue(recvQueue)
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

   std::vector<char> recvbuf(message::Message::MESSAGE_SIZE);
   std::vector<char> sendbuf(message::Message::MESSAGE_SIZE);
   while (!m_done) {

      bool received = false, sent = false;

      {
         message::Message *msg = (message::Message *)recvbuf.data();
         if (!message::recv(socket(), *msg, received))
            break;

         if (received) {
            if (msg->type() == message::Message::MODULEEXIT) {
               std::cerr << "user interface " << id() << " quit" << std::endl;
               assert(msg->senderId() == id());
               m_done = true;
            } else {
               sendQueue().send(*msg);
            }
         }
      }

      {
         message::Message *msg = (message::Message *)sendbuf.data();
         sent = recvQueue().tryReceive(*msg);
         if (sent) {
            if (!message::send(socket(), *msg))
               break;
         }
      }

      if (!received && !sent)
         usleep(10000);
   }

   m_done = true;
}

void UiClient::cancel() {

   m_done = true;
   m_ioService.stop();
}

asio::ip::tcp::socket &UiClient::socket() {

   return *m_socket;
}

message::MessageQueue &UiClient::sendQueue() const {

   return *m_sendQueue;
}

message::MessageQueue &UiClient::recvQueue() const {

   return *m_recvQueue;
}

} // namespace vistle
