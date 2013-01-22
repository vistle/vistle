/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/asio.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>

#ifdef HAVE_READLINE
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "pythonembed.h"
#include "communicator.h"

#define CERR \
   std::cerr << "comm [" << rank << "/" << size << "] "

using namespace boost::interprocess;

namespace asio = boost::asio;

namespace vistle {

Communicator *Communicator::s_singleton = NULL;

std::string strip(const std::string& str) {

   const char *chtmp = str.c_str();

   const char *start=&chtmp[0];
   const char *end=&chtmp[str.size()];

   while (*start && isspace(*start))
      ++start;

   while (end > start && (!*end || isspace(*end)))
      --end;

   return std::string(start, end+1);
}


static std::string history_file() {
   std::string history;
   if (getenv("HOME")) {
      history = getenv("HOME");
      history += "/";
   }
   history += ".vistle_history";
   return history;
}

InteractiveClient::InteractiveClient()
: m_close(false)
, m_quitOnEOF(true)
, m_useReadline(true)
, m_ioService(NULL)
, m_socket(NULL)
{
#ifdef DEBUG
   std::cerr << "Using readline" << std::endl;
#endif
#ifdef HAVE_READLINE
   using_history();
   read_history(history_file().c_str());
#endif
}

InteractiveClient::InteractiveClient(int readfd, int writefd, bool keepInterpreterLock)
: m_close(true)
, m_quitOnEOF(false)
, m_keepInterpreter(keepInterpreterLock)
, m_useReadline(false)
, m_ioService(NULL)
, m_socket(NULL)
{
   m_ioService = new boost::asio::io_service();
   m_socket = new boost::asio::ip::tcp::socket(*m_ioService);
}

InteractiveClient::InteractiveClient(const InteractiveClient &o)
: m_close(o.m_close)
, m_quitOnEOF(o.m_quitOnEOF)
, m_keepInterpreter(o.m_keepInterpreter)
, m_useReadline(o.m_useReadline)
, m_ioService(o.m_ioService)
, m_socket(o.m_socket)
{
   o.m_close = false;
}

InteractiveClient::~InteractiveClient() {

   if (m_useReadline) {
#ifdef HAVE_READLINE
      write_history(history_file().c_str());
      //append_history(::history_length, history_file().c_str());
#endif
   }
   if (m_close) {
      delete m_socket;
      delete m_ioService;
   }
}


bool InteractiveClient::isConsole() const {

   return m_useReadline;
}

void InteractiveClient::setInput(const std::string &input) {

   //std::copy (input.begin(), input.end(), std::back_inserter(readbuf));
}

bool InteractiveClient::readline(std::string &line, bool vistle) {

   const size_t rsize = 1024;
   bool result = true;

   if (m_useReadline) {
#ifdef HAVE_READLINE
      char *l= ::readline(vistle ? "vistle> " : "");
      if (l) {
         line = l;
         if (vistle && !line.empty() && lastline != line) {
            lastline = line;
            add_history(l);
         }
         free(l);
      } else {
         line.clear();
         result = false;
      }
#endif
   } else {

      boost::system::error_code error;
      asio::streambuf buf;
      asio::read_until(socket(), buf, '\n', error);
      if (error) {
         result = false;
      }

      std::istream buf_stream(&buf);

      std::getline(buf_stream, line);
   }

   return result;
}

void InteractiveClient::operator()() {

   Communicator::the().acquireInterpreter(this);
   if (!m_keepInterpreter) {
      printGreeting();
      Communicator::the().releaseInterpreter();
   }

   for (;;) {
      std::string line;
      bool again = readline(line, !m_keepInterpreter);

      line = strip(line);
      if (line == "?" || line == "h" || line == "help") {
         write("Type \"help(vistle)\" for help, \"help()\" for general help\n\n");
      } else if (!line.empty()) {
         if (line == "quit")
            line = "quit()";

         if (line == "exit" || line == "exit()") {
            if (!isConsole()) {
               line.clear();
               break;
            }
         }

         if (!m_keepInterpreter)
            Communicator::the().acquireInterpreter(this);
         Communicator::the().execute(line);
         if (!m_keepInterpreter)
            Communicator::the().releaseInterpreter();
      }

      if (!again) {
         if (m_quitOnEOF) {
            std::cerr << "Quitting..." << std::endl;
            Communicator::the().setQuitFlag();
         }
         break;
      }
      if (!m_keepInterpreter)
         printPrompt();
   }

   if (m_keepInterpreter)
      Communicator::the().releaseInterpreter();
}

void InteractiveClient::setQuitOnEOF() {
   
   m_quitOnEOF = true;
}

bool InteractiveClient::write(const std::string &str) {

   if (isConsole()) {
      size_t n = str.size();
      return ::write(1, &str[0], n) == n;
   }

   return asio::write(socket(), asio::buffer(str));
}

bool InteractiveClient::printPrompt() {

   if (m_useReadline)
      return true;

   return write("vistle> ");
}

bool InteractiveClient::printGreeting() {

   bool ok = write("Type \"help\" for help, \"quit\" to exit\n");
   if (!ok)
      return ok;
   return printPrompt();
}

asio::ip::tcp::socket &InteractiveClient::socket() {

   return *m_socket;
}

static void splitpath(const std::string &value, std::vector<std::string> *components)
{
#ifdef _WIN32
   const char *sep = ";";
#else
   const char *sep = ":";
#endif

   std::string::size_type begin = 0;
   do {
      std::string::size_type end = value.find(sep, begin);

      std::string c;
      if (end != std::string::npos) {
         c = value.substr(begin, end-begin);
         ++end;
      } else {
         c = value.substr(begin);
      }
      begin = end;

      if (!c.empty())
         components->push_back(c);
   } while(begin != std::string::npos);
}

static std::string getbindir(int argc, char *argv[]) {

   char *wd = getcwd(NULL, 0);
   if (!wd) {

      std::cerr << "Communicator: failed to determine working directory: " << strerror(errno) << std::endl;
      exit(1);
   }
   std::string cwd(wd);
   free(wd);

   // determine complete path to executable
   std::string executable;
#ifdef _WIN32
   char buf[2000];
   DWORD sz = GetModuleFileName(NULL, buf, sizeof(buf));
   if (sz != 0) {

      executable = buf;
   } else {

      std::cerr << "Communicator: getbindir(): GetModuleFileName failed - error: " << GetLastError() << std::endl;
   }
#else
   char buf[PATH_MAX];
   ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
   if(len != -1) {

      executable = std::string(buf, len);
   } else if (argc >= 1) {

      bool found = false;
      if (!strchr(argv[0], '/')) {

         if(const char *path = getenv("PATH")) {
            std::vector<std::string> components;
            splitpath(path, &components);
            for(size_t i=0; i<components.size(); ++i) {

               std::string component = components[i];
               if (component[0] != '/')
                  component = cwd + "/" + component;

               DIR *dir = opendir(component.c_str());
               if (!dir) {
                  std::cerr << "Communicator: failed to open directory " << component << ": " << strerror(errno) << std::endl;
                  continue;
               }

               while (struct dirent *ent = readdir(dir)) {
                  if (!strcmp(ent->d_name, argv[0])) {
                     found = true;
                     break;
                  }
               }

               if (found) {
                  executable = component + "/" + argv[0];
                  break;
               }
            }
         }
      } else if (argv[0][0] != '/') {
         executable = cwd + "/" + argv[0];
      } else {
         executable = argv[0];
      }
   }
#endif

   // guess vistle bin directory
   if (!executable.empty()) {

      std::string dir = executable;
#ifdef _WIN32
      std::string::size_type idx = dir.find_last_of("\\/");
#else
      std::string::size_type idx = dir.rfind('/');
#endif
      if (idx == std::string::npos) {

         dir = cwd;
      } else {

         dir = executable.substr(0, idx);
      }

#ifdef DEBUG
      std::cerr << "Communicator: vistle bin directory determined to be " << dir << std::endl;
#endif
      return dir;
   }

   return std::string();
}

Communicator::Communicator(int argc, char *argv[], int r, int s)
   : rank(r), size(s),
   m_quitFlag(false),
   moduleID(0),
     interpreter(NULL),
     mpiReceiveBuffer(NULL), mpiMessageSize(0),
     m_moduleCounter(0),
     m_barrierCounter(0),
     m_activeBarrier(-1),
     m_reachedBarriers(-1),
     m_activeClient(NULL),
#ifdef HAVE_READLINE
     m_console(InteractiveClient()),
#else
     m_console(InteractiveClient(0, 1)),
#endif
     m_consoleThreadCreated(false),
     m_port(8192),
     messageQueue(create_only,
           message::MessageQueue::createName("command_queue", 0, r).c_str(),
           256 /* num msg */, message::Message::MESSAGE_SIZE),
     m_acceptor(m_ioService)
{
   assert(s_singleton == NULL);
   s_singleton = this;

   m_bindir = getbindir(argc, argv);

   mpiReceiveBuffer = new char[message::Message::MESSAGE_SIZE];

   // post requests for length of next MPI message
   MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, 0,
             MPI_COMM_WORLD, &request);
   MPI_Barrier(MPI_COMM_WORLD);
}

Communicator &Communicator::the() {

   assert(s_singleton && "make sure to use the vistle Python module only from within vistle");
   if (!s_singleton)
      exit(1);
   return *s_singleton;
}

int Communicator::getRank() const {

   return rank;
}

int Communicator::getSize() const {

   return size;
}

int Communicator::newModuleID() {
   ++m_moduleCounter;

   return m_moduleCounter;
}

void Communicator::resetModuleCounter() {

   m_moduleCounter = 0;
}

int Communicator::getBarrierCounter() {

   ++m_barrierCounter;
   return m_barrierCounter;
}

boost::mutex &Communicator::barrierMutex() {

   return m_barrierMutex;
}

boost::condition_variable &Communicator::barrierCondition() {

   return m_barrierCondition;
}

void Communicator::barrierReached(int id) {

   assert(id >= 0);
   MPI_Barrier(MPI_COMM_WORLD);
   m_reachedBarriers = id;
   m_activeBarrier = -1;
   reachedSet.clear();
   CERR << "Barrier [" << id << "] reached" << std::endl;
   m_barrierCondition.notify_all();
}

std::vector<int> Communicator::getRunningList() const {

   std::vector<int> result;
   for (RunningMap::const_iterator it = runningMap.begin();
         it != runningMap.end();
         ++it) {
      result.push_back(it->first);
   }
   return result;
}

std::vector<int> Communicator::getBusyList() const {

   std::vector<int> result;
   for (ModuleSet::const_iterator it = busySet.begin();
         it != busySet.end();
         ++it) {
      result.push_back(*it);
   }
   return result;
}

std::string Communicator::getModuleName(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return std::string();

   return it->second;
}


void Communicator::registerInterpreter(PythonEmbed *pi) {

   interpreter = pi;
}

void Communicator::setInput(const std::string &input) {

   m_initialInput = input;
}

void Communicator::setFile(const std::string &filename) {

   m_initialFile = filename;
}

void Communicator::setQuitFlag() {

   m_quitFlag = true;
}

void Communicator::acquireInterpreter(InteractiveClient *client) {

   interpreter_mutex.lock();
   m_activeClient = client;
}

void Communicator::releaseInterpreter() {

   m_activeClient = NULL;
   interpreter_mutex.unlock();
}

InteractiveClient *Communicator::activeClient() const {

   return m_activeClient;
}

bool Communicator::execute(const std::string &line) {
   return interpreter->exec(line);
}

bool Communicator::dispatch() {

   bool done = false;

   if (rank == 0) {

      if (!m_initialFile.empty()) {

         int fd = open(m_initialFile.c_str(), O_RDONLY);
         if (fd == -1) {
            std::cerr << "failed to open " << m_initialFile << std::endl;
         } else {
            boost::thread(InteractiveClient(fd, 1, true));
         }
         m_initialFile.clear();
      } else if (!m_consoleThreadCreated) {
#ifdef DEBUG
            std::cerr << "Creating console thread" << std::endl;
#endif
            m_console.setQuitOnEOF();
            m_console.setInput(m_initialInput);
            m_initialInput.clear();
            m_consoleThread = boost::thread(m_console);
            m_consoleThreadCreated = true;
#ifdef DEBUG
            std::cerr << "Created console thread" << std::endl;
#endif
      }

      if (!done)
         done = m_quitFlag;

      if (m_consoleThreadCreated)
         acceptClients();
   }

   // test for message size from another MPI node
   //    - receive actual message from broadcast
   //    - handle message
   //    - post another MPI receive for size of next message
   int flag;
   MPI_Status status;
   MPI_Test(&request, &flag, &status);

   if (flag) {
      if (status.MPI_TAG == 0) {

         MPI_Bcast(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE,
                   status.MPI_SOURCE, MPI_COMM_WORLD);

         message::Message *message = (message::Message *) mpiReceiveBuffer;
#if 0
         printf("[%02d] message from [%02d] message type %d size %d\n",
                rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
         if (!handleMessage(*message))
            done = true;

         MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, 0,
                   MPI_COMM_WORLD, &request);
      }
   }

   // test for messages from modules
   bool received = false;
   do {
      received = false;
      if (!tryReceiveAndHandleMessage(messageQueue, received, true))
         done = true;

      for (std::map<int, message::MessageQueue *>::iterator i = receiveMessageQueue.begin();
            i != receiveMessageQueue.end();
            ++i) {

         if (reachedSet.find(i->first) != reachedSet.end())
            continue;

         boost::interprocess::message_queue &mq = i->second->getMessageQueue();

         if (!tryReceiveAndHandleMessage(mq, received))
            done = true;

      }
   } while (received);

   return !done;
}

bool Communicator::tryReceiveAndHandleMessage(boost::interprocess::message_queue &mq, bool &received, bool broadcast) {

   bool done = false;
   try {
      size_t msgSize;
      unsigned int priority;
      char msgRecvBuf[vistle::message::Message::MESSAGE_SIZE];

      received = mq.try_receive(
            (void *) msgRecvBuf,
            vistle::message::Message::MESSAGE_SIZE,
            msgSize, priority);

      if (received) {
         if (broadcast) {
            if (!broadcastAndHandleMessage(*(message::Message *)msgRecvBuf))
               done = true;
         } else {
            if (!handleMessage(*(message::Message *)msgRecvBuf))
               done = true;
         }
         return !done;
      }
   } catch (interprocess_exception &ex) {
      CERR << "receive mq " << ex.what() << std::endl;
      exit(-1);
   }

   return !done;
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message) {

   MPI_Request s;
   for (int index = 0; index < size; index ++)
      if (index != rank)
         MPI_Isend(const_cast<unsigned int *>(&message.size), 1, MPI_INT, index, 0,
               MPI_COMM_WORLD, &s);

   MPI_Bcast(const_cast<message::Message *>(&message), message.size, MPI_BYTE, 0, MPI_COMM_WORLD);

   return handleMessage(message);
}


bool Communicator::handleMessage(const message::Message &message) {

#ifdef DEBUG
   CERR << "Message: "
      << "type=" << message.getType() << ", "
      << "size=" << message.getSize() << ", "
      << "from=" << message.getModuleID() << ", "
      << "rank=" << message.getRank() << std::endl;
#endif

   switch (message.getType()) {

      case message::Message::PING: {

         const message::Ping &ping =
            static_cast<const message::Ping &>(message);
         CERR << "Ping [" << ping.getCharacter() << "]" << std::endl;
         for (MessageQueueMap::iterator i = sendMessageQueue.begin();
               i != sendMessageQueue.end();
               ++i) {
            i->second->getMessageQueue().send(&ping, sizeof(ping), 0);
         }
         break;
      }

      case message::Message::PONG: {

         const message::Pong &pong =
            static_cast<const message::Pong &>(message);
         CERR << "Pong [" << pong.getModuleID() << " " << pong.getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         const message::Quit &quit =
            static_cast<const message::Quit &>(message);
         (void) quit;
         return false;
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn &spawn =
            static_cast<const message::Spawn &>(message);
         int moduleID = spawn.getSpawnID();

         std::string name = m_bindir + "/" + spawn.getName();

         std::stringstream modID;
         modID << moduleID;
         std::string id = modID.str();

         std::string smqName =
            message::MessageQueue::createName("smq", moduleID, rank);
         std::string rmqName =
            message::MessageQueue::createName("rmq", moduleID, rank);

         try {
            sendMessageQueue[moduleID] =
               message::MessageQueue::create(smqName);
            receiveMessageQueue[moduleID] =
               message::MessageQueue::create(rmqName);
         } catch (interprocess_exception &ex) {

            CERR << "spawn mq " << ex.what() << std::endl;
            exit(-1);
         }

         std::string executable = name;
         std::vector<const char *> argv;
         if (spawn.getDebugFlag()) {
            CERR << "spawn with debug on rank " << spawn.getDebugRank() << std::endl;
            executable = "debug_vistle.sh";
            argv.push_back(name.c_str());
         }

         MPI_Comm interComm;
         argv.push_back(Shm::the().name().c_str());
         argv.push_back(id.c_str());
         argv.push_back(NULL);

         MPI_Comm_spawn(const_cast<char *>(executable.c_str()), const_cast<char **>(&argv[0]), size, MPI_INFO_NULL,
                        0, MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);

         break;
      }

      case message::Message::STARTED: {

         const message::Started &started =
            static_cast<const message::Started &>(message);
         int moduleID = started.getModuleID();
         runningMap[moduleID] = started.getName();
         break;
      }

      case message::Message::KILL: {

         const message::Kill &kill = static_cast<const message::Kill &>(message);
         MessageQueueMap::iterator it = sendMessageQueue.find(kill.getModule());
         if (it != sendMessageQueue.end())
            it->second->getMessageQueue().send(&kill, sizeof(kill), 0);
         break;
      }

      case message::Message::CONNECT: {

         const message::Connect &connect =
            static_cast<const message::Connect &>(message);
         m_portManager.addConnection(connect.getModuleA(),
                                   connect.getPortAName(),
                                   connect.getModuleB(),
                                   connect.getPortBName());
         break;
      }

      case message::Message::NEWOBJECT: {

         /*
         const message::NewObject *newObject =
            static_cast<const message::NewObject *>(message);
         vistle::Object *object = (vistle::Object *)
            vistle::Shm::the().shm().get_address_from_handle(newObject.getHandle());

         CERR << "NewObject ["
                   << newObject.getHandle() << "] type ["
                   << object.getType() << "] from module ["
                   << newObject.getModuleID() << "]" << std::endl;
         */
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit &moduleExit =
            static_cast<const message::ModuleExit &>(message);
         int mod = moduleExit.getModuleID();

         CERR << " Module [" << mod << "] quit" << std::endl;

         {
            ParameterMap::iterator it = parameterMap.find(mod);
            if (it != parameterMap.end()) {
               parameterMap.erase(it);
            }
         }
         { 
            RunningMap::iterator it = runningMap.find(mod);
            if (it != runningMap.end()) {
               runningMap.erase(it);
            } else {
               CERR << " Module [" << mod << "] not found in map" << std::endl;
            }
         }
         {
            MessageQueueMap::iterator i = sendMessageQueue.find(mod);
            if (i != sendMessageQueue.end()) {
               delete i->second;
               sendMessageQueue.erase(i);
            }
         }
         {
            MessageQueueMap::iterator i = receiveMessageQueue.find(mod);
            i = receiveMessageQueue.find(mod);
            if (i != receiveMessageQueue.end()) {
               delete i->second;
               receiveMessageQueue.erase(i);
            }
         }
         {
            ModuleSet::iterator it = busySet.find(mod);
            if (it != busySet.end())
               busySet.erase(it);
         }
         {
            ModuleSet::iterator it = reachedSet.find(mod);
            if (it != reachedSet.end())
               reachedSet.erase(it);
         }
         m_portManager.removeConnections(mod);
         if (m_activeBarrier >= 0 && reachedSet.size() == sendMessageQueue.size())
            barrierReached(m_activeBarrier);
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute &comp =
            static_cast<const message::Compute &>(message);
         MessageQueueMap::iterator i = sendMessageQueue.find(comp.getModule());
         if (i != sendMessageQueue.end())
            i->second->getMessageQueue().send(&comp, sizeof(comp), 0);
         break;
      }

      case message::Message::BUSY: {

         const message::Busy &busy =
            static_cast<const message::Busy &>(message);

         const int id = busy.getModuleID();
         if (busySet.find(id) != busySet.end()) {
            CERR << "module " << id << " sent Busy twice" << std::endl;
         } else {
            busySet.insert(id);
         }
         break;
      }

      case message::Message::IDLE: {

         const message::Idle &idle =
            static_cast<const message::Idle &>(message);

         const int id = idle.getModuleID();
         ModuleSet::iterator it = busySet.find(id);
         if (it != busySet.end()) {
            busySet.erase(it);
         } else {
            CERR << "module " << id << " sent Idle, but was not busy" << std::endl;
         }
         break;
      }

      case message::Message::CREATEINPUTPORT: {

         const message::CreateInputPort &m =
            static_cast<const message::CreateInputPort &>(message);
         m_portManager.addPort(m.getModuleID(), m.getName(),
                             Port::INPUT);
         break;
      }

      case message::Message::CREATEOUTPUTPORT: {

         const message::CreateOutputPort &m =
            static_cast<const message::CreateOutputPort &>(message);
         m_portManager.addPort(m.getModuleID(), m.getName(),
                             Port::OUTPUT);
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject &m =
            static_cast<const message::AddObject &>(message);
         Object::const_ptr obj = m.takeObject();
         assert(obj->refcount() >= 1);
#if 0
         std::cerr << "Module " << m.getModuleID() << ": "
                   << "AddObject " << m.getHandle() << " (" << obj->getName() << ")"
                   << " ref " << obj->refcount()
                   << " to port " << m.getPortName() << std::endl;
#endif

         Port *port = m_portManager.getPort(m.getModuleID(),
                                          m.getPortName());
         if (port) {
            const std::vector<const Port *> *list =
               m_portManager.getConnectionList(port);

            PortManager::ConnectionList::const_iterator pi;
            for (pi = list->begin(); pi != list->end(); pi ++) {

               MessageQueueMap::iterator mi =
                  sendMessageQueue.find((*pi)->getModuleID());
               if (mi != sendMessageQueue.end()) {
                  const message::AddObject a(m.getModuleID(), m.getRank(),
                                             (*pi)->getName(), obj);
                  const message::Compute c(moduleID, rank,
                                           (*pi)->getModuleID());

                  mi->second->getMessageQueue().send(&a, sizeof(a), 0);
                  mi->second->getMessageQueue().send(&c, sizeof(c), 0);
               }
            }
         }
         else
            std::cerr << "comm [" << rank << "/" << size << "] Addbject ["
                      << m.getHandle() << "] to port ["
                      << m.getPortName() << "]: port not found" << std::endl;

         break;
      }

      case message::Message::SETPARAMETER: {

         const message::SetParameter &m =
            static_cast<const message::SetParameter &>(message);

#ifdef DEBUG
         CERR << "SetParameter: sender=" << m.getModuleID() << ", module=" << m.getModule() << ", name=" << m.getName() << std::endl;
#endif

         if (m.getModuleID() != 0) {
            ModuleParameterMap &pm = parameterMap[m.getModule()];
            ModuleParameterMap::iterator it = pm.find(m.getName());
            if (it == pm.end()) {
               CERR << "no such parameter: module id=" << m.getModule() << ", name=" << m.getName() << std::endl;
            } else {
               Parameter *p = it->second;
               m.apply(p);
            }
         }

         if (m.getModuleID() != m.getModule()) {
            // message to module
            MessageQueueMap::iterator i
               = sendMessageQueue.find(m.getModule());
            if (i != sendMessageQueue.end())
               i->second->getMessageQueue().send(&m, m.getSize(), 0);
         }
         break;
      }

      case message::Message::ADDPARAMETER: {
         
         const message::AddParameter &m =
            static_cast<const message::AddParameter &>(message);

#ifdef DEBUG
         CERR << "AddParameter: module=" << m.getModuleID() << ", name=" << m.getName() << std::endl;
#endif

         ModuleParameterMap &pm = parameterMap[m.getModuleID()];
         ModuleParameterMap::iterator it = pm.find(m.getName());
         if (it != pm.end()) {
            CERR << "double parameter" << std::endl;
         } else {
            pm[m.getName()] = m.getParameter();
         }
         break;
      }

      case message::Message::BARRIER: {

         const message::Barrier &m =
            static_cast<const message::Barrier &>(message);

         CERR << "Barrier [" << m.getBarrierId() << "]" << std::endl;
         assert(m_activeBarrier == -1);
         m_activeBarrier = m.getBarrierId();
         if (sendMessageQueue.empty()) {
            barrierReached(m_activeBarrier);
         } else {
            for (MessageQueueMap::iterator i = sendMessageQueue.begin();
                  i != sendMessageQueue.end();
                  ++i) {
               i->second->getMessageQueue().send(&m, sizeof(m), 0);
            }
         }
         break;
      }

      case message::Message::BARRIERREACHED: {
         const message::BarrierReached &m =
            static_cast<const message::BarrierReached &>(message);
#ifdef DEBUG
         CERR << "BarrierReached [barrier " << m.getBarrierId() << ", module " << m.getModuleID() << "]" << std::endl;
#endif
         if (m_activeBarrier == -1) {
            m_activeBarrier = m.getBarrierId();
         }
         assert(m_activeBarrier == m.getBarrierId());
         reachedSet.insert(m.getModuleID());

         if (reachedSet.size() == receiveMessageQueue.size()) {
            barrierReached(m_activeBarrier);
         }
         break;
      }

      default:

         CERR << "unhandled message from (id "
            << message.getModuleID() << " rank " << message.getRank() << ") "
            << "type " << message.getType()
            << std::endl;

         break;

   }

   return true;
}

Communicator::~Communicator() {

   message::Quit quit(0, rank);

   for (MessageQueueMap::iterator i = sendMessageQueue.begin(); i != sendMessageQueue.end(); ++i)
      i->second->getMessageQueue().send(&quit, sizeof(quit), 1);

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   CERR << "waiting for " << sendMessageQueue.size() << " modules to quit" << std::endl;
   int retries = 10000;
   while (sendMessageQueue.size() > 0 && --retries >= 0) {
      dispatch();
      usleep(1000);
   }

   disconnectClients();

   if (size > 1) {
      int dummy;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD, &s);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      MPI_Wait(&s, MPI_STATUS_IGNORE);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   delete[] mpiReceiveBuffer;
}

void Communicator::disconnectClients() {

}

void Communicator::startAccept() {

      InteractiveClient client(3000);
      m_acceptor.async_accept(client.socket(), boost::bind<void>(&Communicator::handleAccept, this, client, asio::placeholders::error));
}

void Communicator::handleAccept(InteractiveClient &client, const boost::system::error_code &error) {

   if (!error) {
      std::cerr << "new client" << std::endl;
      new boost::thread(client);
   }

   startAccept();
}

int Communicator::acceptClients() {

   if (!m_acceptor.is_open()) {
 
      asio::ip::tcp::endpoint endpoint;
      try {
         endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
      } catch(const std::exception &e) {
         std::cerr << "except endpoint: " << e.what();
      }
      try {
         m_acceptor.open(endpoint.protocol());
      } catch(const std::exception &e) {
         std::cerr << "except open: " << e.what();
      }
      m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
      try {
         m_acceptor.bind(endpoint);
      } catch(std::exception &e) {
         std::cerr << "except bind: " << e.what();
      }

      m_acceptor.listen();

      startAccept();
   }
   m_ioService.poll();

   return -1;
}

const PortManager &Communicator::portManager() const {

   return m_portManager;
}

std::vector<std::string> Communicator::getParameters(int id) const {

   std::vector<std::string> result;

   ParameterMap::const_iterator pit = parameterMap.find(id);
   if (pit == parameterMap.end())
      return result;

   const ModuleParameterMap &pmap = pit->second;
   BOOST_FOREACH (ModuleParameterMap::value_type val, pmap) {
      result.push_back(val.first);
   }

   return result;
}

const Parameter *Communicator::getParameter(int id, const std::string &name) const {

   ParameterMap::const_iterator pit = parameterMap.find(id);
   if (pit == parameterMap.end())
      return NULL;

   ModuleParameterMap::const_iterator mpit = pit->second.find(name);
   if (mpit == pit->second.end())
      return NULL;

   return mpit->second;
}

} // namespace vistle
