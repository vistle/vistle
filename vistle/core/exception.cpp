#include "exception.h"

namespace vistle {

exception::exception(const std::string &what)
: m_what(what)
{

}

exception::~exception() throw() {}

const char* exception::what() const throw() {

   return m_what.c_str();
}

} //namespace vistle
