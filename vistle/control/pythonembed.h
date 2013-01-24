#ifndef PYTHON_EMBED_H
#define PYTHON_EMBED_H

#include <string>
#include <boost/python/object.hpp>

namespace vistle {

class ClientManager;

namespace message {
struct Message;
}

class PythonEmbed {
   public:

      PythonEmbed(ClientManager &manager, const std::string &name);
      ~PythonEmbed();
      static PythonEmbed &the();
      ClientManager &manager() const;

      static void print_output(const std::string &str);
      static void print_error(const std::string &str);
      static std::string raw_input(const std::string &prompt);
      static std::string readline();
      static void handleMessage(const message::Message &message);

      bool exec(const std::string &python);
      bool exec_file(const std::string &filename);
   private:
      ClientManager &m_manager;
      boost::python::object m_namespace;
      static PythonEmbed *s_singleton;

};

} // namespace vistle

#endif
