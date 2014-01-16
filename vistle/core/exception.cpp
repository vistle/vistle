#include "exception.h"
#include <util/tools.h>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace vistle {

namespace except {

exception::exception(const std::string &what)
: m_what(what)
, m_where(vistle::backtrace())
{

#ifndef _WIN32
   char hostname[1024];
   gethostname(hostname, sizeof(hostname));
   m_info = "hostname: ";
   m_info += hostname;
#endif
}

exception::~exception() throw() {}

const char* exception::what() const throw() {

   return m_what.c_str();
}

const char *exception::info() const throw() {

   return m_info.c_str();
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
