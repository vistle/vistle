#include "exception.h"
#include <util/tools.h>

namespace vistle {

namespace except {

exception::exception(const std::string &what)
: m_what(what)
{

   m_where = vistle::backtrace();
}

exception::~exception() throw() {}

const char* exception::what() const throw() {

   return m_what.c_str();
}

const char* exception::where() const throw() {

   return m_where.c_str();
}


not_implemented::not_implemented(const std::string &what)
: exception(what)
{

}

} //namespace exception

} //namespace vistle
