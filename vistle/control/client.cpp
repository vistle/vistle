#ifdef HAVE_READLINE
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "client.h"
#include "communicator.h"

#include <boost/algorithm/string/trim.hpp>

namespace asio = boost::asio;

namespace vistle {

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
: m_close(true)
, m_quitOnEOF(false)
, m_keepInterpreter(false)
, m_prompt("vistle> ")
{
}

InteractiveClient::InteractiveClient(const InteractiveClient &o)
: m_close(o.m_close)
, m_quitOnEOF(o.m_quitOnEOF)
, m_keepInterpreter(o.m_keepInterpreter)
, m_prompt(o.m_prompt)
{
   o.m_close = false;
}

InteractiveClient::~InteractiveClient() {
}


bool InteractiveClient::isConsole() const {

   return false;
}

void InteractiveClient::setInput(const std::string &input) {

   //std::copy (input.begin(), input.end(), std::back_inserter(readbuf));
}

void InteractiveClient::setKeepInterpreterLock() {

   m_keepInterpreter = true;
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

      boost::trim(line);
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
   }

   if (m_keepInterpreter)
      Communicator::the().releaseInterpreter();
}

void InteractiveClient::setQuitOnEOF() {
   
   m_quitOnEOF = true;
}

bool InteractiveClient::printGreeting() {

   return write("Type \"help\" for help, \"quit\" to exit\n");
}

ReadlineClient::ReadlineClient()
: InteractiveClient()
{
#ifdef DEBUG
   std::cerr << "Using readline" << std::endl;
#endif
#ifdef HAVE_READLINE
   using_history();
   read_history(history_file().c_str());
#endif
}

ReadlineClient::ReadlineClient(const ReadlineClient &o)
: InteractiveClient(o)
{
   o.m_close = false;
}

ReadlineClient::~ReadlineClient() {

   if (m_close) {
#ifdef HAVE_READLINE
      write_history(history_file().c_str());
      //append_history(::history_length, history_file().c_str());
#endif
   }
}

bool ReadlineClient::isConsole() const {

   return true;
}

bool ReadlineClient::readline(std::string &line, bool vistle) {

   bool result = true;

#ifdef HAVE_READLINE
   char *l= ::readline(vistle ? m_prompt.c_str() : "");
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

   return result;
}

bool ReadlineClient::write(const std::string &str) {

   std::cout << str;
   return true;
}

AsioClient::AsioClient()
: m_ioService(NULL)
, m_socket(NULL)
{
   m_ioService = new boost::asio::io_service();
   m_socket = new boost::asio::ip::tcp::socket(*m_ioService);
}

AsioClient::AsioClient(const AsioClient &o)
: InteractiveClient(o)
, m_ioService(o.m_ioService)
, m_socket(o.m_socket)
{
   o.m_close = false;
}
AsioClient::~AsioClient() {

   if (m_close) {
      delete m_socket;
      delete m_ioService;
   }
}

bool AsioClient::readline(std::string &line, bool vistle) {

   bool result = true;

   if (vistle)
      printPrompt();

   boost::system::error_code error;
   asio::streambuf buf;
   asio::read_until(socket(), buf, '\n', error);
   if (error) {
      result = false;
   }

   std::istream buf_stream(&buf);
   std::getline(buf_stream, line);

   return result;
}

bool AsioClient::write(const std::string &str) {

   return asio::write(socket(), asio::buffer(str));
}

bool InteractiveClient::printPrompt() {

   return true;
}

bool AsioClient::printPrompt() {

   return write(m_prompt);
}

asio::ip::tcp::socket &AsioClient::socket() {

   return *m_socket;
}

} // namespace vistle
