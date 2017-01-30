#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <iostream>
#include <list>
#include <map>
#include <exception>

#include "export.h"
#include <core/parameter.h>
#include <core/port.h>
#include <core/porttracker.h>
#include <core/statetracker.h>
#include <core/message.h>

#include <boost/asio.hpp>
#include <mutex>
#include <condition_variable>

namespace vistle {

class V_UIEXPORT UserInterface {

 public:
   UserInterface(const std::string &host, const unsigned short port, StateObserver *observer=nullptr);
   virtual ~UserInterface();

   void stop();

   virtual bool dispatch();
   bool sendMessage(const message::Message &message);

   int id() const;
   const std::string &host() const;

   const std::string &remoteHost() const;
   unsigned short remotePort() const;

   boost::asio::ip::tcp::socket &socket();
   const boost::asio::ip::tcp::socket &socket() const;

   bool tryConnect();
   bool isConnected() const;

   StateTracker &state();

   bool getLockForMessage(const message::uuid_t &uuid);
   bool getMessage(const message::uuid_t &uuid, message::Message &msg);

   void registerObserver(StateObserver *observer);

 protected:

   int m_id;
   std::string m_hostname;
   std::string m_remoteHost;
   unsigned short m_remotePort;
   bool m_isConnected;

   StateTracker m_stateTracker;

   bool handleMessage(const message::Message *message);

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::socket m_socket;

   struct RequestedMessage {
      std::mutex mutex;
      std::condition_variable cond;
      bool received;
      std::vector<char> buf;

      RequestedMessage(): received(false) {}
   };

   typedef std::map<message::uuid_t, std::shared_ptr<RequestedMessage>> MessageMap;
   MessageMap m_messageMap;
   std::mutex m_messageMutex; //< protect access to m_messageMap
   bool m_locked;
   std::vector<message::Buffer> m_sendQueue;
};

} // namespace vistle

#endif
