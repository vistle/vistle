#include "exception.h"

namespace vistle {

namespace except {

exception::exception(const std::string &what)
: m_what(what)
{

}

exception::~exception() throw() {}

const char* exception::what() const throw() {

   return m_what.c_str();
}


not_implemented::not_implemented(const std::string &what)
: exception(what)
{

}

} //namespace exception

} //namespace vistle
