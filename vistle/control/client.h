#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>

namespace vistle {

class InteractiveClient {
   public:
      InteractiveClient();
      InteractiveClient(const InteractiveClient &o);
      virtual ~InteractiveClient();

      void operator()();

      virtual bool isConsole() const;

      void setQuitOnEOF();
      void setKeepInterpreterLock();
      void setInput(const std::string &input);

      virtual bool write(const std::string &s) = 0;
      virtual bool readline(std::string &line, bool vistle=true) = 0;

   protected:
      virtual bool printPrompt();

      std::string m_prompt;
      mutable bool m_close;
      bool printGreeting();
      bool m_quitOnEOF;
      bool m_keepInterpreter;
};

class AsioClient: public InteractiveClient {
   public:
      AsioClient();
      AsioClient(const AsioClient &o);
      ~AsioClient();

      bool readline(std::string &line, bool vistle=true);
      bool write(const std::string &s);

      boost::asio::ip::tcp::socket &socket();
   protected:
      bool printPrompt();

      boost::asio::io_service *m_ioService;
      boost::asio::ip::tcp::socket *m_socket;
};

class ReadlineClient: public InteractiveClient {
   public:
      ReadlineClient();
      ReadlineClient(const ReadlineClient &o);
      ~ReadlineClient();

      bool readline(std::string &line, bool vistle=true);
      bool write(const std::string &s);
      bool isConsole() const;

   private:
      std::string lastline;
};

} // namespace vistle

#endif
