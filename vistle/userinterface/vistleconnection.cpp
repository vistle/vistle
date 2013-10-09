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
#include <userinterface/userinterface.h>

namespace vistle {

class VistleConnectionLocker: public VistleConnection::Locker {

public:
   VistleConnectionLocker(boost::recursive_mutex &mtx)
   : m_mutex(mtx)
   {
      m_mutex.lock();
   }

   ~VistleConnectionLocker() {

      m_mutex.unlock();
   }

   private:
   boost::recursive_mutex &m_mutex;
};

VistleConnection *VistleConnection::s_instance = nullptr;

/*************************************************************************/
// begin class VistleConnection
/*************************************************************************/

VistleConnection::VistleConnection(vistle::UserInterface &ui)
: m_ui(ui)
, m_done(false)
, m_quitOnExit(false)
{
   assert(s_instance == nullptr);
   s_instance = this;
}

VistleConnection::~VistleConnection() {

   if (m_quitOnExit) {
      sendMessage(message::Quit());
   }

   s_instance = nullptr;
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
      {
         mutex_lock lock(m_mutex);
         if (m_done) {
            break;
         }
      }
      usleep(10000);
   }
}

void VistleConnection::setQuitOnExit(bool quit) {

   m_quitOnExit = quit;
}

vistle::UserInterface &VistleConnection::ui() const {

   return m_ui;
}

void VistleConnection::lock() {

   m_mutex.lock();
}

void VistleConnection::unlock() {

   m_mutex.unlock();
}

std::unique_ptr<VistleConnection::Locker> VistleConnection::locked() {

   return std::unique_ptr<Locker>(new VistleConnectionLocker(m_mutex));
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

void vistle::VistleConnection::sendParameter(const Parameter *p) const
{
   mutex_lock lock(m_mutex);
   vistle::message::SetParameter set(p->module(), p->getName(), p);
   sendMessage(set);
}

bool VistleConnection::requestReplyAsync(const vistle::message::Message &send) const {

   if (!ui().getLockForMessage(send.uuid()))
      return false;

   sendMessage(send);
   return true;
}

bool VistleConnection::waitForReplyAsync(const vistle::message::Message::uuid_t &uuid, vistle::message::Message &reply) const {

   return ui().getMessage(uuid, reply);
}

bool VistleConnection::waitForReply(const vistle::message::Message &send, vistle::message::Message &reply) const {

   if (!requestReplyAsync(send)) {
      return false;
   }
   return waitForReplyAsync(send.uuid(), reply);
}

std::vector<std::string> vistle::VistleConnection::getParameters(int id) const
{
   mutex_lock lock(m_mutex);
   return ui().state().getParameters(id);
}

int vistle::VistleConnection::barrier() const {

   std::vector<char> msgBuf(message::Message::MESSAGE_SIZE);
   message::Message *msg = (message::Message *)msgBuf.data();

   message::Barrier m(0);
   if (!waitForReply(m, *msg)) {
      return false;
   }

   switch(msg->type()) {
      case message::Message::BARRIERREACHED: {
         const message::BarrierReached *reached = static_cast<const message::BarrierReached *>(msg);
         return reached->getBarrierId();
         break;
      }
      default:
         assert("expected BarrierReached message" == 0);
         break;
   }

   return -1;
}

void vistle::VistleConnection::resetDataFlowNetwork() const
{
   {
      mutex_lock lock(m_mutex);
      for (int id: ui().state().getRunningList()) {
         sendMessage(message::Kill(id));
      }
   }
#if 0
   int barrierId = barrier();
   if (barrierId < 0) {
      std::cerr << "VistleConnection::resetDataFlowNetwork: barrier failed" << std::endl;
      return;
   }
   sendMessage(message::ResetModuleIds());
#endif
}

void VistleConnection::executeSources() const
{
   mutex_lock lock(m_mutex);
   for (int id: ui().state().getRunningList()) {
      auto inputs = ui().state().portTracker()->getInputPorts(id);
      bool isSource = true;
      for (auto input: inputs) {
         if (!input->connections().empty())
            isSource = false;
      }
      if (isSource)
         sendMessage(message::Compute(id, -1));
   }
}

void VistleConnection::connect(const Port *from, const Port *to) const {

   message::Connect conn(from->getModuleID(), from->getName(), to->getModuleID(), to->getName());
   sendMessage(conn);
}

void VistleConnection::disconnect(const Port *from, const Port *to) const {

   message::Disconnect disc(from->getModuleID(), from->getName(), to->getModuleID(), to->getName());
   sendMessage(disc);
}

} //namespace vistle
