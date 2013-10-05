 /*
  QPyConsole.cpp

  Controls the GEMBIRD Silver Shield PM USB outlet device

  (C) 2006, Mondrian Nuessle, Computer Architecture Group, University of Mannheim, Germany
  
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 
   nuessle@uni-mannheim.de

 */

// modified by YoungTaek Oh.

#ifdef WIN32
#   undef _DEBUG
#endif
#include <Python.h>
#include <boost/python.hpp>
#include "vistleconsole.h"
#include <userinterface/pythonmodule.h>

#include <QApplication>
#include <QDebug>

namespace bp = boost::python;

namespace gui {

static bp::object glb, loc;

static QString resultString;

class Redirector {
   bool m_stderr = false;

public:
   Redirector(bool error = false)
      : m_stderr(error)
   {}

   void write(const std::string &output)
   {
      //std::cerr << (m_stderr?"ERR: ":"OUT: ") << output << std::flush;
      QString outputString = QString::fromStdString(output);
      if (m_stderr)
         VistleConsole::the()->setTextColor(VistleConsole::the()->errColor());
      else
         VistleConsole::the()->setTextColor(VistleConsole::the()->outColor());
      VistleConsole::the()->insertPlainText(outputString);
      VistleConsole::the()->setTextColor(VistleConsole::the()->cmdColor());
      VistleConsole::the()->ensureCursorVisible();
      QApplication::processEvents();
   }
};

BOOST_PYTHON_MODULE(_redirector)
{
   using namespace boost::python;
   class_<Redirector>("redirector")
         .def(init<>())
         .def(init<bool>())
         .def("write", &Redirector::write, "implement the write method to redirect stdout/err");
};

static void clear()
{
    VistleConsole::the()->clear();
}

static void pyreset()
{
    VistleConsole::the()->reset();
}

static void save(const std::string &filename)
{
    VistleConsole::the()->saveScript(QString::fromStdString(filename));
}

static void load(const std::string &filename)
{
    VistleConsole::the()->loadScript(QString::fromStdString(filename));
}

static void history()
{
    VistleConsole::the()->printHistory();
}

static void quit()
{
    resultString="Use reset() to restart the interpreter; otherwise exit your application\n";
}

static std::string raw_input(const std::string &prompt)
{
   VistleConsole::the()->setTextColor(VistleConsole::the()->outColor());
   VistleConsole::the()->append(resultString);
   resultString = "";
   VistleConsole::the()->setPrompt(QString::fromStdString(prompt));
   QCoreApplication::processEvents();
   QCoreApplication::flush();
   QCoreApplication::sendPostedEvents();
   std::string ret = VistleConsole::the()->getRawInput().toStdString();
   VistleConsole::the()->setTextColor(VistleConsole::the()->cmdColor());
   VistleConsole::the()->setNormalPrompt(false);
   return ret;
}

BOOST_PYTHON_MODULE(_console)
{
   using namespace boost::python;

   def("clear", clear, "clear the console");
   def("reset", pyreset, "reset the interpreter and clear the console");
   def("save", save, "save commands up to now in given file");
   def("load", load, "load commands from given file");
   def("history", history, "shows the history");
   def("quit", quit, "print information about quitting");
   def("raw_input", raw_input, "handle raw input");
}

void VistleConsole::printHistory()
{
    uint index = 1;
    for ( QStringList::Iterator it = history.begin(); it != history.end(); ++it )
    {
        resultString.append(QString("%1\t%2\n").arg(index).arg(*it));
        index ++;
    }
}

void VistleConsole::appendInfo(const QString &text)
{
   setTextColor(errColor());
   append(text);
   setTextColor(cmdColor());
}

void VistleConsole::appendDebug(const QString &text)
{
   setTextColor(outColor());
   append(text);
   setTextColor(cmdColor());
}

VistleConsole *VistleConsole::s_instance = NULL;

VistleConsole *VistleConsole::the()
{
   assert(s_instance);
   return s_instance;
}

//QTcl console constructor (init the QTextEdit & the attributes)
VistleConsole::VistleConsole(QWidget *parent)
   : QConsole(parent, "Type \"help(vistle)\" for help, \"help()\" for general help ")
   , lines(0)
{
   assert(!s_instance);
   s_instance = this;

    //set the Python Prompt
    setNormalPrompt(true);

    Py_Initialize();
    /* NOTE: In previous implementaion, local name and global name
             were allocated separately.  And it causes a problem that
             a function declared in this console cannot be called.  By
             unifying global and local name with __main__.__dict__, we
             can get more natural python console.
    */
    bp::object main_module = bp::import("__main__");
    loc = glb = main_module.attr("__dict__");

    //PyImport_AddModule("_redirector");
    init_redirector();

    //PyImport_AddModule("_console");
    init_console();

    bp::import("rlcompleter");

    PyRun_SimpleString("import sys\n"

                       "import _redirector\n"
                       "sys.stdout = _redirector.redirector()\n"
                       "sys.stderr = _redirector.redirector(True)\n"

                       "sys.path.insert(0, \".\")\n" // add current path

                       "import __builtin__\n"

                       "import _console\n"
                       "__builtin__.clear=_console.clear\n"
                       "__builtin__.reset=_console.reset\n"
                       "__builtin__.save=_console.save\n"
                       "__builtin__.load=_console.load\n"
                       "__builtin__.history=_console.history\n"
                       //"__builtin__.quit=_console.quit\n"
                       //"__builtin__.exit=_console.quit\n"
                       "__builtin__.raw_input=_console.raw_input\n"

                       "import rlcompleter\n"
                       "__builtin__.completer=rlcompleter.Completer()\n"
        );
}
char save_error_type[1024], save_error_info[1024];

bool
VistleConsole::py_check_for_unexpected_eof()
{
    PyObject *errobj, *errdata, *errtraceback, *pystring;

    /* get latest python exception info */
    PyErr_Fetch(&errobj, &errdata, &errtraceback);

    pystring = NULL;
    if (errobj != NULL &&
        (pystring = PyObject_Str(errobj)) != NULL &&     /* str(object) */
        (PyString_Check(pystring))
        )
    {
        strcpy(save_error_type, PyString_AsString(pystring));
    }
    else
        strcpy(save_error_type, "<unknown exception type>");
    Py_XDECREF(pystring);

    pystring = NULL;
    if (errdata != NULL &&
        (pystring = PyObject_Str(errdata)) != NULL &&
        (PyString_Check(pystring))
        )
        strcpy(save_error_info, PyString_AsString(pystring));
    else
        strcpy(save_error_info, "<unknown exception data>");
    Py_XDECREF(pystring);

    if (strstr(save_error_type, "exceptions.SyntaxError")!=NULL &&
        strncmp(save_error_info,"('unexpected EOF while parsing',",32)==0)
    {
        return true;
    }
    PyErr_Print ();
    resultString="Error: ";
    resultString.append(save_error_info);
    Py_XDECREF(errobj);
    Py_XDECREF(errdata);         /* caller owns all 3 */
    Py_XDECREF(errtraceback);    /* already NULL'd out */
    return false;
}

//Desctructor
VistleConsole::~VistleConsole()
{
    Py_Finalize();
}

//Call the Python interpreter to execute the command
//retrieve back results using the python internal stdout/err redirectory (see above)
QString VistleConsole::interpretCommand(const QString &command, int *res)
{
    PyObject* py_result;
    PyObject* dum;
    bool multiline=false;
    *res = 0;
    if (!command.startsWith('#') && (!command.isEmpty() || (command.isEmpty() && lines!=0)))
    {
        this->command.append(command);
        py_result=Py_CompileString(this->command.toLocal8Bit().data(),"<stdin>",Py_single_input);
        if (py_result==0)
        {
            multiline=py_check_for_unexpected_eof();
            if (!multiline) {
                if (command.endsWith(':'))
                    multiline = true;
            }

            if (multiline)
            {
                setMultilinePrompt(false);
                this->command.append("\n");
                lines++;
                resultString="";
                QConsole::interpretCommand(command, res);
                return "";
            }
            else
            {
                setNormalPrompt(false);
                *res=-1;
                QString result=resultString;
                resultString="";
                QConsole::interpretCommand(command, res);
                this->command="";
                this->lines=0;
                return result;
            }
        }

        if ( (lines!=0 && command=="") || (this->command!="" && lines==0))
        {
            setNormalPrompt(false);
            this->command="";
            this->lines=0;

            dum = PyEval_EvalCode ((PyCodeObject *)py_result, glb.ptr(), loc.ptr());
            Py_XDECREF (dum);
            Py_XDECREF (py_result);
            if (PyErr_Occurred ())
            {
                *res=-1;
                PyErr_Print ();
            }
            QString result=resultString;
            resultString="";
            if (command!="")
                QConsole::interpretCommand(command, res);
            return result;
        }
        else if (lines!=0 && command!="") //following multiliner line
        {
            this->command.append("\n");
            *res=0;
            QConsole::interpretCommand(command, res);
            return "";
        }
        else
        {
            return "";
        }

    }
    else
        return "";
}

QStringList VistleConsole::suggestCommand(const QString &cmd, QString& prefix)
{
   Q_UNUSED(prefix);

   QStringList completions;
   if (!cmd.isEmpty()) {
      for (int n=0; ; ++n) {
         std::stringstream str;
         str << "completer.complete(\"" << cmd.toStdString() << "\"," << n << ")" << std::endl;
         try {
            boost::python::object ret = boost::python::eval(str.str().c_str(), glb, loc);
            if (ret == boost::python::object())
               break;
            std::string result = boost::python::extract<std::string>(ret);
            QString completed = QString::fromStdString(result).trimmed();
            if (!completed.isEmpty())
               completions.append(completed);
         } catch(boost::python::error_already_set const &) {
            PyErr_Print();
            break;
         }
      }
   }
   completions.removeDuplicates();
   return completions;
}

} // namespace gui
