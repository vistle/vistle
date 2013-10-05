#ifndef UICLIENT_H
#define UICLIENT_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <core/message.h>
#include <core/messagequeue.h>
#include "export.h"

namespace vistle {

class UiManager;

class V_CONTROLEXPORT UiClient {
   public:
      UiClient(UiManager &manager, int id, boost::shared_ptr<message::MessageQueue> sendQueue, boost::shared_ptr<message::MessageQueue> recvQueue);
      ~UiClient();

      int id() const;
      void operator()();
      void cancel();
      bool done() const;
      UiManager &manager() const;

      message::MessageQueue &sendQueue() const;
      message::MessageQueue &recvQueue() const;

      boost::asio::ip::tcp::socket &socket();

   private:
      UiClient(const UiClient &o);

      int m_id;
      bool m_done;
      boost::asio::io_service m_ioService;
      boost::scoped_ptr<boost::asio::ip::tcp::socket> m_socket;
      UiManager &m_manager;
      boost::shared_ptr<message::MessageQueue> m_sendQueue, m_recvQueue;
};

} // namespace vistle

#endif
