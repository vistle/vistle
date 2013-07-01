#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


#include <core/message.h>
#include "pythonembed.h"
#include "pythonmodule.h"
#include "userinterface.h"

namespace bp = boost::python;

namespace vistle {

#if 0
namespace detail {
   template <typename T, typename U>
      struct to_tuple {
         static PyObject* convert(std::pair<T,U> const& p) {
            using namespace boost::python;
            return incref(boost::python::make_tuple(p.first, p.second).ptr());
         }

         static PyTypeObject const *get_pytype() { return &PyTuple_Type; }
      };
}

template <typename T, typename U>
struct to_tuple {
   to_tuple() {
      using namespace boost::python;
      to_python_converter<std::pair<T,U>, detail::to_tuple<T,U>
#ifdef BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
         , true
#endif
         >();
   }
};

template <typename T, typename U>
struct from_tuple
{
   from_tuple() {
      using namespace boost::python::converter;
      registry::push_back(&convertible,
            &construct,
            boost::python::type_id<std::pair<T,U> >()
#ifdef BOOST_PYTHON_SUPPORTS_PY_SIGNATURES
            , get_pytype
#endif
            );
   }

   static const PyTypeObject *get_pytype() { return &PyTuple_Type; }

   static void *convertible(PyObject *o) {
      using namespace boost::python;
      if (!PyTuple_Check(o) || PyTuple_GET_SIZE(o) != 2) return 0;
      return o;
   }

   static void construct(
         PyObject *o,
         boost::python::converter::rvalue_from_python_stage1_data *data)
   {
      using boost::python::extract;
      using namespace boost::python::converter;
      PyObject *first  = PyTuple_GET_ITEM(o, 0),
               *second = PyTuple_GET_ITEM(o, 1);
      void *storage =
         ((rvalue_from_python_storage<std::pair<T,U> >*) data)->storage.bytes;
      new (storage) std::pair<T,U>(extract<T>(first), extract<U>(second));
      data->convertible = storage;
   }
};

template <typename T, typename U>
struct to_and_from_tuple
{
   to_and_from_tuple() {
      to_tuple<T,U>();
      from_tuple<T,U>();
   }
};
#endif


PythonEmbed *PythonEmbed::s_singleton = NULL;

PythonEmbed::PythonEmbed(UserInterface &ui, const std::string &name)
: m_ui(ui)
{
   assert(s_singleton == NULL);
   s_singleton = this;

   static char namebuf[1024];
   strncpy(namebuf, name.c_str(), sizeof(namebuf));
   namebuf[sizeof(namebuf)-1] = '\0';
   Py_SetProgramName(namebuf);
   Py_Initialize();

   bp::class_<std::vector<int> >("vector<int>")
      .def(bp::vector_indexing_suite<std::vector<int> >());

   bp::class_<std::vector<std::string> >("vector<string>")
      .def(bp::vector_indexing_suite<std::vector<std::string> >());

   bp::class_<std::vector<std::pair<int, std::string> > >("vector<pair<int,string>>")
      .def(bp::vector_indexing_suite<std::vector<std::pair<int, std::string> > >());

   bp::class_<std::pair<int, std::string> >("pair<int,string>")
      .def_readwrite( "first", &std::pair< int, std::string >::first, "first value" ) 
      .def_readwrite( "second", &std::pair< int, std::string >::second, "second value" ); 

   bp::class_<ParameterVector<double> >("ParameterVector<double>")
      .def(bp::vector_indexing_suite<ParameterVector<double> >());

   bp::object main = bp::import("__main__");
   m_namespace = main.attr("__dict__");

   PythonModule::import(&m_namespace);
}


PythonEmbed::~PythonEmbed() {

    Py_Finalize();
}

PythonEmbed &PythonEmbed::the() {

   assert(s_singleton);
   return *s_singleton;
}

UserInterface &PythonEmbed::ui() const {

   return m_ui;
}

StateTracker &PythonEmbed::state() {

   return m_ui.state();
}

bool PythonEmbed::exec(const std::string &python) {

   bool ok = false;
   try {
      bp::object r = bp::exec(python.c_str(), m_namespace, m_namespace);
      if (r != bp::object()) {
         m_namespace["__result__"] = r;
         bp::exec("print __result__");
      }
      ok = true;
   } catch (bp::error_already_set) {
      //std::cerr << "Python exec error" << std::endl;
      PyErr_Print();
      PyErr_Clear();
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;
}

bool PythonEmbed::exec_file(const std::string &filename) {

   bool ok = false;
   try {
      bp::object r = bp::exec_file(filename.c_str(), m_namespace, m_namespace);
      if (r != bp::object()) {
         m_namespace["__result__"] = r;
         bp::exec("print __result__");
      }
      ok = true;
   } catch (bp::error_already_set) {
      //std::cerr << "Python exec error" << std::endl;
      PyErr_Print();
      PyErr_Clear();
      ok = false;
   } catch (std::exception &ex) {
      std::cerr << "Unknown Python exec error: " << ex.what() << std::endl;
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;
}

std::string PythonEmbed::raw_input(const std::string &prompt) {

#if 0
   Client *c = the().manager().activeClient();
   std::string line;
   if (InteractiveClient *ic = dynamic_cast<InteractiveClient *>(c)) {
      ic->write(prompt);
      std::string line;
      ic->readline(line);
      char *end = &line[line.size()-1];
      if (end > &line[0] && *end == '\n') {
         --end;
         if (end > &line[0] && *end == '\r')
            --end;
      }

      return std::string(&line[0], end+1);
   }
#endif

   std::cerr << "raw_input: not implemented" << std::endl;
   return std::string();
}

std::string PythonEmbed::readline() {

#if 0
   Client *c = the().manager().activeClient();
   std::string line;
   if (InteractiveClient *ic = dynamic_cast<InteractiveClient *>(c)) {
      ic->readline(line, false);
   }

   return line;
#else
   std::cerr << "readline: not implemented" << std::endl;
   std::string line;
   std::getline(std::cin, line);
   return line;
#endif
}

void PythonEmbed::print_output(const std::string &str) {

#if 0
   //std::cout << "OUT: " << str << std::endl;
   Client *c = the().manager().activeClient();
   if (InteractiveClient *ic = dynamic_cast<InteractiveClient *>(c)) {
      ic->write(str);
   } else {
      std::cout << str;
   }
#else
      std::cout << str << std::flush;
#endif
}

void PythonEmbed::print_error(const std::string &str) {

#if 0
   //std::cerr << "ERR: " << str << std::endl;
   Client *c = the().manager().activeClient();
   InteractiveClient *ic = dynamic_cast<InteractiveClient *>(c);
   if (ic && !ic->isConsole())
      ic->write(str);
   else
      std::cerr << str;
#else
      std::cerr << str << std::flush;
#endif
}

bool PythonEmbed::handleMessage(const message::Message &message) {

   return PythonEmbed::the().ui().sendMessage(message);
}

bool PythonEmbed::waitForReply(const message::Message &send, message::Message &reply) {

   UserInterface &ui = PythonEmbed::the().ui();
   boost::unique_lock<boost::mutex> lock(ui.mutexForMessage(send.uuid()));
   handleMessage(send);
   ui.conditionForMessage(send.uuid()).wait(lock);
   return ui.getMessage(send.uuid(), reply);
}

} // namespace vistle
