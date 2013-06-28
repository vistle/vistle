#ifndef UICLIENT_H
#define UICLIENT_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <core/message.h>
#include <core/messagequeue.h>

namespace vistle {

class UiManager;

class UiClient {
   public:
      UiClient(UiManager &manager, boost::shared_ptr<message::MessageQueue> sendQueue, boost::shared_ptr<message::MessageQueue> recvQueue);
      ~UiClient();

      void operator()();
      void cancel();
      bool done() const;
      UiManager &manager() const;
      bool recv(message::Message &msg, bool &received);
      bool send(const message::Message &msg);

      boost::interprocess::message_queue &sendQueue() const;
      boost::interprocess::message_queue &recvQueue() const;

      boost::asio::ip::tcp::socket &socket();

   private:
      boost::asio::io_service m_ioService;
      boost::asio::ip::tcp::socket *m_socket;
      UiManager &m_manager;
      bool m_done;
      UiClient(const UiClient &o);
      boost::shared_ptr<message::MessageQueue> m_sendQueue, m_recvQueue;
};

} // namespace vistle

#endif
