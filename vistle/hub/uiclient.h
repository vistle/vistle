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
      UiClient(UiManager &manager, int id, boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
      ~UiClient();

      int id() const;
      void cancel();
      bool done() const;
      UiManager &manager() const;
      boost::shared_ptr<boost::asio::ip::tcp::socket> socket();

   private:
      UiClient(const UiClient &o);

      int m_id;
      bool m_done;
      boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
      UiManager &m_manager;
};

} // namespace vistle

#endif
