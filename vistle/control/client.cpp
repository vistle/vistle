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

Client::Client()
: m_done(false)
{
}

Client::~Client() {
}

bool Client::done() const {

   return m_done;
}

LockedClient::LockedClient()
: m_locker(Communicator::the().interpreterMutex())
{
   std::cerr << "new LockedClient: " << this << std::endl;
}

LockedClient::~LockedClient()
{
   std::cerr << "delete LockedClient: " << this << std::endl;
}

FileClient::FileClient(const std::string &filename)
: m_filename(filename)
{
}

void FileClient::cancel() {
}

void FileClient::operator()() {

   Communicator::the().executeFile(m_filename);

   m_done = true;
}

BufferClient::BufferClient(const std::string &buf)
: m_buffer(buf)
{
}

void BufferClient::cancel() {
}

void BufferClient::operator()() {

   Communicator::the().execute(m_buffer);

   m_done = true;
}


InteractiveClient::InteractiveClient()
: m_close(true)
, m_quitOnEOF(false)
, m_prompt("vistle> ")
{
}

InteractiveClient::~InteractiveClient() {
}


bool InteractiveClient::isConsole() const {

   return false;
}

void InteractiveClient::operator()() {

   printGreeting();

   for (;;) {
      std::string line;
      bool again = readline(line);
      boost::trim(line);

      if (line == "?" || line == "h" || line == "help") {
         write("Type \"help(vistle)\" for help, \"help()\" for general help\n\n");
      } else if (!line.empty()) {
         if (line == "quit")
            line = "quit()";

         if (line == "exit" || line == "exit()") {
            if (!isConsole()) {
               line.clear();
               again = false;
               break;
            }
         }

         {
            boost::lock_guard<boost::mutex>(Communicator::the().interpreterMutex());
            Communicator::the().execute(line, this);
         }
      }

      if (!again) {
         if (m_quitOnEOF) {
            std::cerr << "Quitting..." << std::endl;
            Communicator::the().setQuitFlag();
         }
         break;
      }
   }

   m_done = true;
}

bool InteractiveClient::printGreeting() {

   return write("Type \"help\" for help, \"quit\" to exit\n");
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


ReadlineClient::ReadlineClient()
: InteractiveClient()
{
   m_quitOnEOF = true;
#ifdef DEBUG
   std::cerr << "Using readline" << std::endl;
#endif
#ifdef HAVE_READLINE
   using_history();
   read_history(history_file().c_str());
#endif
}

ReadlineClient::~ReadlineClient() {

   if (m_close) {
#ifdef HAVE_READLINE
      write_history(history_file().c_str());
      //append_history(::history_length, history_file().c_str());
#endif
   }
}

void ReadlineClient::cancel() {

   //fclose(stdin);
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
: m_socket(new asio::ip::tcp::socket(m_ioService))
{
}

AsioClient::~AsioClient() {

   if (m_close) {
      delete m_socket;
   }
}

void AsioClient::cancel() {

   m_ioService.stop();
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
