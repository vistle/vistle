#ifdef HAVE_READLINE
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "client.h"
#include "communicator.h"

namespace asio = boost::asio;

namespace vistle {

static std::string strip(const std::string& str) {

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

} // namespace vistle
