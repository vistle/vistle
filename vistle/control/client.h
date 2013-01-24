#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>

namespace vistle {

class ClientManager;

class Client {
   public:
      Client(ClientManager &manager);
      virtual ~Client();

      virtual void operator()() = 0;
      virtual void cancel() = 0;
      bool done() const;
      ClientManager &manager() const;

   protected:
      bool m_done;
      ClientManager &m_manager;
   private:
      Client(const Client &o);
};

class LockedClient: public Client {
   public:
      LockedClient(ClientManager &manager);
      ~LockedClient();

   private:
      boost::lock_guard<boost::mutex> m_locker;
};

class FileClient: public LockedClient {
   public:
      FileClient(ClientManager &manager, const std::string &filename);

      virtual void operator()();
      void cancel();

   private:
      std::string m_filename;
};

class BufferClient: public LockedClient {
   public:
      BufferClient(ClientManager &manager, const std::string &buffer);

      virtual void operator()();
      void cancel();

   private:
      std::string m_buffer;
};

class InteractiveClient: public Client {
   public:
      InteractiveClient(ClientManager &manager);
      virtual ~InteractiveClient();

      void operator()();

      virtual bool isConsole() const;

      virtual bool write(const std::string &s) = 0;
      virtual bool readline(std::string &line, bool vistle=true) = 0;

   protected:
      virtual bool printPrompt();

      std::string m_prompt;
      mutable bool m_close;
      bool printGreeting();
      bool m_quitOnEOF;
};

class AsioClient: public InteractiveClient {
   public:
      AsioClient(ClientManager &manager);
      ~AsioClient();

      void cancel();
      bool readline(std::string &line, bool vistle=true);
      bool write(const std::string &s);

      boost::asio::ip::tcp::socket &socket();
   protected:
      bool printPrompt();

      boost::asio::io_service m_ioService;
      boost::asio::ip::tcp::socket *m_socket;
};

class ReadlineClient: public InteractiveClient {
   public:
      ReadlineClient(ClientManager &manager);
      ~ReadlineClient();

      void cancel();
      bool readline(std::string &line, bool vistle=true);
      bool write(const std::string &s);
      bool isConsole() const;

   private:
      std::string lastline;
};

} // namespace vistle

#endif
