#ifndef PYTHON_EMBED_H
#define PYTHON_EMBED_H

#include <string>
#include <iostream>
#include <boost/python/object.hpp>

#include <core/porttracker.h>
#include <core/statetracker.h>

namespace vistle {

class UserInterface;
class StateTracker;

namespace message {
class Message;
}

class PythonEmbed {
   public:

      PythonEmbed(UserInterface &ui, const std::string &name);
      ~PythonEmbed();
      static PythonEmbed &the();
      UserInterface &ui() const;
      StateTracker &state();

      static void print_output(const std::string &str);
      static void print_error(const std::string &str);
      static std::string raw_input(const std::string &prompt);
      static std::string readline();
      static bool handleMessage(const message::Message &message);
      static bool waitForReply(const message::Message &send, message::Message &reply);

      bool exec(const std::string &python);
      bool exec_file(const std::string &filename);
   private:
      UserInterface &m_ui;
      boost::python::object m_namespace;
      static PythonEmbed *s_singleton;

};

} // namespace vistle

#endif
