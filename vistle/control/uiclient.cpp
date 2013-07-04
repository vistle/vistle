#include "uiclient.h"
#include <core/tcpmessage.h>

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

      message::Message *msg = (message::Message *)recvbuf.data();
      bool received = false;
      if (!message::recv(socket(), *msg, received))
         break;

      if (received) {
         sendQueue().send(msg, msg->size(), 0);
      }


      size_t msgSize;
      unsigned int priority;
      received = recvQueue().try_receive(sendbuf.data(), message::Message::MESSAGE_SIZE, msgSize, priority);
      if (received) {
         message::Message *msg = (message::Message *)sendbuf.data();
         if (!message::send(socket(), *msg))
            break;
      }

      usleep(10000);
   }

   m_done = true;
}

void UiClient::cancel() {

   m_ioService.stop();
}

asio::ip::tcp::socket &UiClient::socket() {

   return *m_socket;
}

boost::interprocess::message_queue &UiClient::sendQueue() const {

   return m_sendQueue->getMessageQueue();
}

boost::interprocess::message_queue &UiClient::recvQueue() const {

   return m_recvQueue->getMessageQueue();
}

} // namespace vistle
