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


#include <cstdio>
#ifdef HAVE_PYTHON
#include <pybind11/eval.h>
#include <pybind11/embed.h>
#include <vistle/util/pybind.h>
#endif
#include "vistleconsole.h"
#ifdef HAVE_PYTHON
#include <vistle/python/pythonmodule.h>
#include <vistle/python/pythoninterface.h>
#endif
#include <vistle/core/message.h>
#include <vistle/core/messages.h>

#include <QApplication>
#include <QDebug>

#ifdef HAVE_PYTHON
namespace py = pybind11;
#define USE_RLCOMPLETER
#endif

namespace gui {

static QString resultString;

#ifdef HAVE_PYTHON
class Redirector {
    bool m_stderr;

public:
    Redirector(bool error = false): m_stderr(error) {}

    void write(const std::string &output)
    {
        if (VistleConsole::the()) {
            QString outputString = QString::fromStdString(output);
            if (m_stderr)
                VistleConsole::the()->setTextColor(VistleConsole::the()->errColor());
            else
                VistleConsole::the()->setTextColor(VistleConsole::the()->outColor());
            VistleConsole::the()->insertPlainText(outputString);
            VistleConsole::the()->setTextColor(VistleConsole::the()->cmdColor());
            VistleConsole::the()->ensureCursorVisible();
            QApplication::processEvents();
        } else {
            std::cerr << (m_stderr ? "ERR: " : "OUT: ") << output << std::flush;
        }
    }
};

PYBIND11_EMBEDDED_MODULE(_redirector, m)
{
    py::class_<Redirector>(m, "redirector")
        .def(py::init<>())
        .def(py::init<bool>())
        .def("write", &Redirector::write, "implement the write method to redirect stdout/err");
}
#endif

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
    resultString = "Use reset() to restart the interpreter; otherwise exit your application\n";
}

static std::string raw_input(const std::string &prompt)
{
    VistleConsole::the()->setTextColor(VistleConsole::the()->outColor());
    VistleConsole::the()->append(resultString);
    resultString = "";
    VistleConsole::the()->setPrompt(QString::fromStdString(prompt));
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents();
    std::string ret = VistleConsole::the()->getRawInput().toStdString();
    VistleConsole::the()->setTextColor(VistleConsole::the()->cmdColor());
    VistleConsole::the()->setNormalPrompt(false);
    return ret;
}

#ifdef HAVE_PYTHON
PYBIND11_EMBEDDED_MODULE(_console, m)
{
    m.def("clear", clear, "clear the console");
    m.def("reset", pyreset, "reset the interpreter and clear the console");
    m.def("save", save, "save commands up to now in given file");
    m.def("load", load, "load commands from given file");
    m.def("history", history, "shows the history");
    m.def("quit", quit, "print information about quitting");
    m.def("raw_input", raw_input, "handle raw input");
}
#endif

void VistleConsole::printHistory()
{
    uint index = 1;
    for (QStringList::Iterator it = history.begin(); it != history.end(); ++it) {
        resultString.append(QString("%1\t%2\n").arg(index).arg(*it));
        index++;
    }
}

void VistleConsole::appendHtml(const QString &text, int type)
{
    using namespace vistle::message;

    // save the current command
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString saved = cursor.selectedText();
    cursor.removeSelectedText();

    // remove last line
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    cursor.movePosition(QTextCursor::End);

    if (type == SendText::Info) {
        setTextColor(infoColor());
    } else if (type == SendText::Cerr) {
        setTextColor(errColor());
    }
    insertHtml(text);

    // reinsert it
    cursor.movePosition(QTextCursor::End);
    setTextColor(cmdColor());
    append(saved);
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void VistleConsole::appendInfo(const QString &text, int type)
{
    using namespace vistle::message;

    // save the current command
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString saved = cursor.selectedText();
    cursor.removeSelectedText();

    // remove last line
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    cursor.movePosition(QTextCursor::End);

    if (type == SendText::Info) {
        setTextColor(infoColor());
    } else if (type == SendText::Cerr) {
        setTextColor(errColor());
    }
    append(text);

    // reinsert it
    cursor.movePosition(QTextCursor::End);
    setTextColor(cmdColor());
    append(saved);
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void VistleConsole::appendDebug(const QString &text)
{
    appendInfo(text, -1);
}

void VistleConsole::setNormalPrompt(bool display)
{
    setPrompt("> ", display);
}

VistleConsole *VistleConsole::s_instance = NULL;

VistleConsole *VistleConsole::the()
{
    assert(s_instance);
    return s_instance;
}

//QTcl console constructor (init the QTextEdit & the attributes)
VistleConsole::VistleConsole(QWidget *parent)
#ifdef HAVE_PYTHON
: QConsole(parent, "<p>Type \"<tt>help(vistle)</tt>\" for help, \"<tt>help()</tt>\" for general help</p>")
#else
: QConsole(parent)
#endif
, lines(0)
{
    assert(!s_instance);
    s_instance = this;

#ifdef HAVE_PYTHON
    //set the Python Prompt
    setNormalPrompt(true);
#endif
}

void VistleConsole::init()
{
#ifdef HAVE_PYTHON

    /* NOTE: In previous implementation, local name and global name
             were allocated separately.  And it causes a problem that
             a function declared in this console cannot be called.  By
             unifying global and local name with __main__.__dict__, we
             can get more natural python console.
    */
#ifdef USE_RLCOMPLETER
    try {
        py::module::import("rlcompleter");
    } catch (...) {
        std::cerr << "error importing rlcompleter" << std::endl;
    }
#endif

    try {
        std::string pyCode = "import sys\n"

                             "import _redirector\n"
                             "sys.stdout = _redirector.redirector()\n"
                             "sys.stderr = _redirector.redirector(True)\n"

                             "sys.path.insert(0, \".\")\n" // add current path

                             "import _console\n"

                             "import builtins\n"
                             "builtins.clear=_console.clear\n"
                             "builtins.reset=_console.reset\n"
                             "builtins.save=_console.save\n"
                             "builtins.load=_console.load\n"
                             "builtins.history=_console.history\n"
                             //"builtins.quit=_console.quit\n"
                             //"builtins.exit=_console.quit\n"
                             "builtins.input=_console.raw_input\n"

#ifdef USE_RLCOMPLETER
                             "import rlcompleter\n"
                             "builtins.completer=rlcompleter.Completer()\n"
#endif
            ;
        PyRun_SimpleString(pyCode.c_str());

    } catch (...) {
        std::cerr << "error running Python initialisation" << std::endl;
    }
#endif
}

void VistleConsole::finish()
{
#ifdef HAVE_PYTHON
    m_locals.reset();
#endif
}

#ifdef HAVE_PYTHON
namespace {

std::string python2String(PyObject *obj, bool *ok = nullptr)
{
    std::string result;
    if (ok)
        *ok = false;
    PyObject *str = PyObject_Str(obj);
    if (!str) {
        //result = "NULL object";
    } else if (PyUnicode_Check(str)) {
        PyObject *bytes = PyUnicode_AsEncodedString(str, "ASCII", "strict"); // Owned reference
        if (bytes) {
            if (ok)
                *ok = true;
            // does not work for python3 result = PyBytes_AS_STRING(bytes); // Borrowed pointer
            result = PyBytes_AsString(bytes);
            Py_DECREF(bytes);
        } else {
            result = "<cannot interpret unicode>";
        }
    } else if (PyBytes_Check(str)) {
        if (ok)
            *ok = true;
        result = PyBytes_AsString(str);
    } else {
        //return "object neither Byte nor Unicode";
        return "";
    }

    if (str)
        Py_XDECREF(str);
    return result;
}

} // anonymous namespace

bool VistleConsole::py_check_for_unexpected_eof()
{
    PyObject *errobj, *errdata, *errtraceback;

    /* get latest python exception info */
    PyErr_Fetch(&errobj, &errdata, &errtraceback);
    std::string save_error_type = python2String(errobj);
    if (save_error_type.empty()) {
        save_error_type = "<unknown exception type>";
    }
    std::string save_error_info = python2String(errdata);
    if (save_error_info.empty()) {
        save_error_info = "<unknown exception data>";
    }
    if (save_error_type.find("exceptions.SyntaxError") != std::string::npos &&
        save_error_info.find("('unexpected EOF while parsing',") == 0) {
        return true;
    }
    PyErr_Print();
    PyErr_Clear();
    resultString = "Error: ";
    resultString.append(save_error_info.c_str());
    Py_XDECREF(errobj);
    Py_XDECREF(errdata); /* caller owns all 3 */
    Py_XDECREF(errtraceback); /* already NULL'd out */
    return false;
}
#endif

//Destructor
VistleConsole::~VistleConsole()
{
    s_instance = nullptr;
}

//Call the Python interpreter to execute the command
//retrieve back results using the python internal stdout/err redirectory (see above)
QString VistleConsole::interpretCommand(const QString &command, int *res)
{
#ifdef HAVE_PYTHON
    PyObject *py_result = nullptr;
    bool multiline = false;
    *res = 0;
    if (command.startsWith('#') || (command.isEmpty() && lines == 0))
        return "";

    this->command.append(command);
    py_result = Py_CompileString(this->command.toLocal8Bit().data(), "<stdin>", Py_single_input);
    if (!py_result) {
        multiline = py_check_for_unexpected_eof();
        if (!multiline) {
            if (command.endsWith(':'))
                multiline = true;
        }

        if (multiline) {
            setMultilinePrompt(false);
            this->command.append("\n");
            lines++;
            resultString = "";
            QConsole::interpretCommand(command, res);
            return "";
        } else {
            *res = -1;
            QString result = resultString;
            resultString = "";
            QConsole::interpretCommand(command, res);
            setNormalPrompt(false);
            this->command = "";
            this->lines = 0;
            return result;
        }
    }

    if ((lines != 0 && command == "") || (this->command != "" && lines == 0)) {
        setNormalPrompt(false);
        this->command = "";
        this->lines = 0;

#if PY_VERSION_HEX >= 0x03000000
        PyObject *dum = PyEval_EvalCode(py_result, py::globals().ptr(), locals().ptr());
#else
        PyObject *dum = nullptr;
        if (PyCode_Check(py_result)) {
            dum = PyEval_EvalCode((PyCodeObject *)py_result, py::globals().ptr(), locals().ptr());
        }
#endif
        Py_XDECREF(dum);
        Py_XDECREF(py_result);
        if (PyErr_Occurred()) {
            *res = -1;
            PyErr_Print();
            PyErr_Clear();
        }
        QString result = resultString;
        resultString = "";
        if (command != "")
            QConsole::interpretCommand(command, res);
        return result;
    } else if (lines != 0 && command != "") //following multiliner line
    {
        this->command.append("\n");
        *res = 0;
        QConsole::interpretCommand(command, res);
        return "";
    }
#endif

    return "";
}

#ifdef HAVE_PYTHON
pybind11::object &VistleConsole::locals()
{
    if (!m_locals) {
        m_locals.reset(new py::object());
    }
    if (!m_locals->ptr()) {
        try {
            auto main_module = py::module::import("__main__");
            *m_locals = main_module.attr("__dict__");
        } catch (...) {
            std::cerr << "error importing __main__" << std::endl;
        }
    }
    return *m_locals.get();
}
#endif

QStringList VistleConsole::suggestCommand(const QString &cmd, QString &prefix)
{
    prefix = "";

    QStringList completions;
#ifdef HAVE_PYTHON
    if (!cmd.isEmpty()) {
        for (int n = 0;; ++n) {
            std::stringstream str;
            str << "completer.complete(\"" << cmd.toStdString() << "\"," << n << ")" << std::endl;
            try {
                py::object ret = py::eval<py::eval_expr>(str.str(), py::globals(), locals());
                if (!ret.ptr()) {
                    std::cerr << "VistleConsole::suggestCommand: no valid Python object" << std::endl;
                    break;
                }
                if (ret.is_none()) {
                    break;
                }
                std::string result = py::str(ret);
                QString completed = QString::fromStdString(result).trimmed();
                if (!completed.isEmpty())
                    completions.append(completed);
            } catch (py::error_already_set const &ex) {
                std::cerr << "VistleConsole::suggestCommand: Python error: " << ex.what() << std::endl;
                PyErr_Print();
                PyErr_Clear();
                break;
            } catch (py::cast_error const &ex) {
                std::cerr << "VistleConsole::suggestCommand: Python cast error: " << ex.what() << std::endl;
                break;
            } catch (std::exception const &ex) {
                std::cerr << "VistleConsole::suggestCommand: Unknown error: " << ex.what() << std::endl;
                break;
            } catch (...) {
                std::cerr << "VistleConsole::suggestCommand: Unknown error" << std::endl;
                break;
            }
        }
    }
    completions.removeDuplicates();
#endif
    return completions;
}

} // namespace gui
