/*********************************************************************************/
/*! \file handler.cpp
 *
 * Contains three classes:
 * 1. VistleConnection -- simple class, handling commands dispatched to vistle's userInterface.
 * 2. VistleObserver -- observer class that watches for changes in vistle, and sends
 *    signals to the MainWindow.
 */
/**********************************************************************************/
#include "vistleconnection.h"

namespace vistle {

VistleConnection *VistleConnection::s_instance = nullptr;

/*************************************************************************/
// begin class VistleConnection
/*************************************************************************/

VistleConnection::VistleConnection(vistle::UserInterface &ui) : m_ui(ui)
{
   assert(s_instance == nullptr);
   s_instance = this;
}

VistleConnection &VistleConnection::the() {

   assert(s_instance);
   return *s_instance;
}

void VistleConnection::cancel() {
   mutex_lock lock(m_mutex);
   m_done = true;
}

void VistleConnection::operator()() {
   while(m_ui.dispatch()) {
      mutex_lock lock(m_mutex);
      if (m_done) {
         break;
      }
      usleep(10000);
   }
}

vistle::UserInterface &VistleConnection::ui() const {

   return m_ui;
}

void VistleConnection::sendMessage(const vistle::message::Message &msg) const
{
   mutex_lock lock(m_mutex);
   ui().sendMessage(msg);
}

vistle::Parameter *VistleConnection::getParameter(int id, const std::string &name) const
{
   mutex_lock lock(m_mutex);
   vistle::Parameter *p = ui().state().getParameter(id, name);
   if (!p) {
      std::cerr << "no such parameter: " << id << ":" << name << std::endl;
   }
   return p;
}

bool VistleConnection::requestReplyAsync(const vistle::message::Message &send) {

   boost::mutex &mutex = ui().mutexForMessage(send.uuid());
   mutex.lock();
   sendMessage(send);
   return true;
}

bool VistleConnection::waitForReplyAsync(const vistle::message::Message::uuid_t &uuid, vistle::message::Message &reply) {

   return ui().getMessage(uuid, reply);
}

bool VistleConnection::waitForReply(const vistle::message::Message &send, vistle::message::Message &reply) {

   if (!requestReplyAsync(send)) {
      return false;
   }
   return waitForReplyAsync(send.uuid(), reply);
}

} //namespace vistle
