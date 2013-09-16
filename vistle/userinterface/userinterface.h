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
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace vistle {

class V_UIEXPORT UserInterface {

 public:
   UserInterface(const std::string &host, const unsigned short port, StateObserver *observer=nullptr);
   virtual ~UserInterface();

   virtual bool dispatch();
   bool sendMessage(const message::Message &message) const;

   int id() const;
   std::string host() const;

   boost::asio::ip::tcp::socket &socket() const;

   StateTracker &state();

   bool getLockForMessage(const message::Message::uuid_t &uuid);
   bool getMessage(const message::Message::uuid_t &uuid, message::Message &msg);

   void registerObserver(StateObserver *observer);

 protected:

   std::string m_hostname;
   int m_id = -1;

   PortTracker m_portTracker;
   StateTracker m_stateTracker;

   bool handleMessage(const message::Message *message);

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::socket *m_socket;

   struct RequestedMessage {
      boost::mutex mutex;
      boost::condition_variable cond;
      bool received = false;
      std::vector<char> buf;
   };

   typedef std::map<message::Message::uuid_t, RequestedMessage> MessageMap;
   MessageMap m_messageMap;
   boost::mutex m_messageMutex; //< protect access to m_messageMap
};

} // namespace vistle

#endif
