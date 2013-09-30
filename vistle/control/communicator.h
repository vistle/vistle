#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <mpi.h>

#include <core/message.h>

#include "export.h"

namespace bi = boost::interprocess;

namespace vistle {

namespace message {
   class Message;
   class MessageQueue;
}

class Parameter;
class PythonEmbed;
class ClientManager;
class UiManager;
class ModuleManager;

class V_CONTROLEXPORT Communicator {
   friend class PythonEmbed;
   friend class ModuleManager;

 public:
   Communicator(int argc, char *argv[], int rank, const std::vector<std::string> &hosts);
   ~Communicator();
   static Communicator &the();

   void setInput(const std::string &input);
   void setFile(const std::string &filename);

   bool dispatch();
   bool handleMessage(const message::Message &message);
   bool forwardToMaster(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   bool sendMessage(int receiver, const message::Message &message) const;
   void setQuitFlag();

   int getRank() const;
   int getSize() const;

   unsigned short uiPort() const;

   ModuleManager &moduleManager() const;

 private:
   void sendUi(const message::Message &message) const;
   ClientManager *m_clientManager;
   UiManager *m_uiManager;
   ModuleManager *m_moduleManager;

   std::string m_initialFile;
   std::string m_initialInput;

   const int m_rank;
   const int m_size;

   bool m_quitFlag;

   int m_recvSize;
   std::vector<char> m_recvBufTo0, m_recvBufToAny;
   MPI_Request m_reqAny, m_reqToRank0;

   typedef std::map<int, message::MessageQueue *> MessageQueueMap;
   boost::shared_ptr<message::MessageQueue> m_commandQueue;
   bool tryReceiveAndHandleMessage(message::MessageQueue &mq, bool &received, bool broadcast=false);

   static Communicator *s_singleton;
};

} // namespace vistle

#endif
