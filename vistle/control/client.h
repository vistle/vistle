#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>

namespace vistle {

class InteractiveClient {
   public:
      InteractiveClient();
      InteractiveClient(int readfd, int writefd=-1 /* same as readfd */, bool keepInterpreterLock=false);
      InteractiveClient(const InteractiveClient &o);
      ~InteractiveClient();

      void operator()();

      bool isConsole() const;

      void setQuitOnEOF();
      void setInput(const std::string &input);

      bool write(const std::string &s);
      bool readline(std::string &line, bool vistle=true);

      boost::asio::ip::tcp::socket &socket();

   private:
      mutable bool m_close;
      bool printPrompt();
      bool printGreeting();
      bool m_quitOnEOF;
      bool m_keepInterpreter;
      bool m_useReadline;
      std::string lastline;
      boost::asio::io_service *m_ioService;
      boost::asio::ip::tcp::socket *m_socket;
};

} // namespace vistle

#endif
