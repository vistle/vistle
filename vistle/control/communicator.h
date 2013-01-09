#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <deque>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <mpi.h>

#include "portmanager.h"

namespace bi = boost::interprocess;

namespace vistle {

namespace message {
   struct Message;
   class MessageQueue;
}

class Parameter;

class PythonEmbed;

class Communicator {
   friend class PythonEmbed;

 public:
   Communicator(int argc, char *argv[], int rank, int size);
   ~Communicator();
   static Communicator &the();
   int currentClient() const;

   void registerInterpreter(PythonEmbed *pi);
   void setInput(const std::string &input);
   void setFile(const std::string &filename);

   bool dispatch();
   bool handleMessage(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   void setQuitFlag();

   int getRank() const;
   int getSize() const;

   int newModuleID();
   std::vector<int> getRunningList() const;
   std::vector<int> getBusyList() const;
   std::string getModuleName(int id) const;

   const PortManager &portManager() const;

 private:

   std::string m_bindir;

   std::string m_initialFile;
   std::string m_initialInput;

   const int rank;
   const int size;

   bool m_quitFlag;

   std::vector<std::vector<char> > readbuf, writebuf;
   std::vector<int> sockfd;
   int moduleID;
   PythonEmbed *interpreter;

   char *mpiReceiveBuffer;
   int mpiMessageSize;

   MPI_Request request;

   typedef std::map<int, message::MessageQueue *> MessageQueueMap;
   MessageQueueMap sendMessageQueue;
   MessageQueueMap receiveMessageQueue;

   std::map<int, bi::shared_memory_object *> shmObjects;

   typedef std::map<int, std::string> RunningMap;
   RunningMap runningMap;
   typedef std::set<int> ModuleSet;
   ModuleSet busySet;

   typedef std::map<std::string, Parameter *> ModuleParameterMap;
   typedef std::map<int, ModuleParameterMap> ParameterMap;
   ParameterMap parameterMap;

   PortManager m_portManager;
   int m_moduleCounter;

   int m_currentClient;
   int checkClients();
   bool setClientBlocking(int num, bool block);
   void allocateBuffers(int num);
   void flushClient(int num);
   void writeClient(int num, const void *buf, size_t n);
   void writeClient(int num, const std::string &s);
   void printGreeting(int num);
   void printPrompt(int num);
   ssize_t readClient(int num, void *buf, size_t n);
   bool hasClientLine(int num);
   std::string readClientLine(int num);
   ssize_t fillClientBuffer(int num);
   void disconnectClients();
   unsigned short m_port;

   static Communicator *s_singleton;

   static const int FdOut = 1;
   enum { StdInOut = 0, Server = 1, SocketStart = 2 };
};

} // namespace vistle

#endif
