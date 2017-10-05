#include <userinterface/pythoninterface.h>
#include <userinterface/pythonmodule.h>
#include <userinterface/userinterface.h>
#include <userinterface/vistleconnection.h>
#include <util/directory.h>

#include <functional>
#include <thread>
#include <mutex>

#include <util/sleep.h>

using namespace vistle;
namespace dir = vistle::directory;

class UiRunner {

 public:
   UiRunner(UserInterface &ui)
      : m_ui(ui)
      , m_done(false)
      {
      }
   void cancel() {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_done = true;
   }
   void operator()() {

      while(m_ui.dispatch()) {
         {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_done)
               break;
         }
      }
      {
         std::unique_lock<std::mutex> lock(m_mutex);
         m_done = true;
      }
   }

 private:
   UserInterface &m_ui;
   bool m_done;
   std::mutex m_mutex;
};

class StatePrinter: public StateObserver {

 public:
   StatePrinter(std::ostream &out)
      : m_out(out)
      {}

   void moduleAvailable(int hub, const std::string &name, const std::string &path) {
       m_out << "   hub: " << hub << ", module: " << name << " (" << path << ")" << std::endl;
   }

   void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName) {
      (void)spawnUuid;
      m_out << "   module " << moduleName << " started: " << moduleId << std::endl;
   }

   void deleteModule(int moduleId) {
      m_out << "   module deleted: " << moduleId << std::endl;
   }

   void moduleStateChanged(int moduleId, int stateBits) {
      m_out << "   module state change: " << moduleId << " (";
      if (stateBits & StateObserver::Initialized) m_out << "I";
      if (stateBits & StateObserver::Killed) m_out << "K";
      if (stateBits & StateObserver::Busy) m_out << "B";
      m_out << ")" << std::endl;
   }

   void newParameter(int moduleId, const std::string &parameterName) {
      m_out << "   new parameter: " << moduleId << ":" << parameterName << std::endl;
   }

   void deleteParameter(int moduleId, const std::string &parameterName) {
      m_out << "   delete parameter: " << moduleId << ":" << parameterName << std::endl;
   }

   void parameterValueChanged(int moduleId, const std::string &parameterName) {
      m_out << "   parameter value changed: " << moduleId << ":" << parameterName << std::endl;
   }

   void parameterChoicesChanged(int moduleId, const std::string &parameterName) {
      m_out << "   parameter choices changed: " << moduleId << ":" << parameterName << std::endl;
   }

   void newPort(int moduleId, const std::string &portName) {
      m_out << "   new port: " << moduleId << ":" << portName << std::endl;
   }

   void deletePort(int moduleId, const std::string &portName) {
      m_out << "   delete port: " << moduleId << ":" << portName << std::endl;
   }

   void newConnection(int fromId, const std::string &fromName,
         int toId, const std::string &toName) {
      m_out << "   new connection: "
         << fromId << ":" << fromName << " -> "
         << toId << ":" << toName << std::endl;
   }

   void deleteConnection(int fromId, const std::string &fromName,
         int toId, const std::string &toName) {
      m_out << "   connection removed: "
         << fromId << ":" << fromName << " -> "
         << toId << ":" << toName << std::endl;
   }

   void info(const std::string &text, message::SendText::TextType textType, int senderId, int senderRank, message::Type refType, const message::uuid_t &refUuid) {

      std::cerr << senderId << "(" << senderRank << "): " << text << std::endl;
   }

 private:
   std::ostream &m_out;
};

int main(int argc, char *argv[]) {

   try {

      std::string host = "localhost";
      unsigned short port = 31093;

      bool quitOnExit = false;
      if (argc > 1) {
         std::string arg(argv[1]);
         if (arg == "-from-vistle") {
            quitOnExit = true;
            argv[1] = argv[0];
            --argc;
            ++argv;
         }
      }

      if (argc > 1)
         host = argv[1];

      if (argc > 2)
         port = atoi(argv[2]);

      std::cerr << "trying to connect UI to " << host << ":" << port << std::endl;
      StatePrinter printer(std::cout);
      UserInterface ui(host, port, &printer);
      PythonInterface python("blower");
      VistleConnection conn(ui);
      conn.setQuitOnExit(quitOnExit);
      PythonModule pythonmodule(&conn);
      python.init();
      pythonmodule.import(&python.nameSpace(), dir::share(dir::prefix(argc, argv)));
      std::thread runnerThread(std::ref(conn));

      while(!std::cin.eof() && !conn.done()) {
         std::string line;
         std::getline(std::cin, line);
         if (line == "exit")
            break;
         python.exec(line);
      }

      std::cerr << "canceling connection" << std::endl;
      conn.cancel();
      std::cerr << "joining thread" << std::endl;
      runnerThread.join();
      std::cerr << "thread joined" << std::endl;

   } catch (vistle::except::exception &ex) {

      std::cerr << "exception: " << ex.what() << std::endl << ex.where() << std::endl;
      return 1;
   } catch (std::exception &ex) {

      std::cerr << "exception: " << ex.what() << std::endl;
      return 1;
   } catch (...) {
      std::cerr << "unknown exception" << std::endl;
      return 1;
   }

   return 0;
}
