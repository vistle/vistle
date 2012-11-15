#ifndef PYTHON_EMBED_H
#define PYTHON_EMBED_H

#include <string>
#include <boost/python/object.hpp>

namespace vistle {

class PythonEmbed {
   public:

      PythonEmbed(int argc, char *argv[]);
      ~PythonEmbed();
      static PythonEmbed &the();

      static void print_output(const char *str);
      static void print_error(const char *str);

      bool exec(const std::string &python);
   private:
      boost::python::object m_namespace;
      static PythonEmbed *s_singleton;

};

} // namespace vistle

#endif
