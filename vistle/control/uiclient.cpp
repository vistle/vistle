#include "uiclient.h"

namespace asio = boost::asio;

namespace vistle {

UiClient::UiClient(UiManager &manager, boost::shared_ptr<message::MessageQueue> sendQueue, boost::shared_ptr<message::MessageQueue> recvQueue)
: m_done(false)
, m_socket(new asio::ip::tcp::socket(m_ioService))
, m_manager(manager)
, m_sendQueue(sendQueue)
, m_recvQueue(recvQueue)
{
}

UiClient::~UiClient() {

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
      if (!recv(*msg, received))
         break;

      if (received) {
         sendQueue().send(msg, msg->size(), 0);
      }


      size_t msgSize;
      unsigned int priority;
      received = recvQueue().try_receive(sendbuf.data(), message::Message::MESSAGE_SIZE, msgSize, priority);
      if (received) {
         message::Message *msg = (message::Message *)sendbuf.data();
         if (!send(*msg))
            break;
      }

      usleep(10000);
   }

   m_done = true;
}

void UiClient::cancel() {

   m_ioService.stop();
}

bool UiClient::recv(message::Message &msg, bool &received) {

   uint32_t sz = 0;

   boost::asio::socket_base::bytes_readable command(true);
   socket().io_control(command);
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
      asio::read(socket(), szbuf);
      sz = ntohl(sz);
      auto msgbuf = asio::buffer(&msg, sz);
      asio::read(socket(), msgbuf);
   } catch (std::exception &ex) {
      std::cerr << "UiClient:recv: exception: " << ex.what() << std::endl;
      received = false;
      result = false;
   }

   return result;
}

bool UiClient::send(const message::Message &msg) {

   try {
      const uint32_t sz = htonl(msg.size());
      auto szbuf = asio::buffer(&sz, sizeof(sz));
      asio::write(socket(), szbuf);
      auto msgbuf = asio::buffer(&msg, msg.size());
      return asio::write(socket(), msgbuf);
   } catch (std::exception &ex) {
      std::cerr << "UiClient:send: exception: " << ex.what() << std::endl;
      return false;
   }
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
