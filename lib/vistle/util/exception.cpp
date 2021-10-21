#include "exception.h"
#include "tools.h"
#include "hostname.h"

namespace vistle {

namespace except {

exception::exception(const std::string &what): m_what(what), m_where(vistle::backtrace())
{
    m_info += hostname();
}

exception::~exception() throw()
{}

const char *exception::what() const throw()
{
    return m_what.c_str();
}

const char *exception::info() const throw()
{
    return m_info.c_str();
}

const char *exception::where() const throw()
{
    return m_where.c_str();
}


not_implemented::not_implemented(const std::string &what): exception(what)
{}

consistency_error::consistency_error(const std::string &what): exception(what)
{}

assertion_failure::assertion_failure(const std::string &what): exception(what)
{}

parent_died::parent_died(const std::string &what): exception(what)
{}

} // namespace except

} //namespace vistle
