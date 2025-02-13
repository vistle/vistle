#ifndef VISTLE_UTIL_EXCEPTION_H
#define VISTLE_UTIL_EXCEPTION_H

#include <exception>
#include <string>

#include "export.h"

namespace vistle {

namespace except {

class V_UTILEXPORT exception: public std::exception {
public:
    exception(const std::string &what = "vistle error");
    virtual ~exception() throw();

    virtual const char *what() const throw();
    virtual const char *info() const throw();
    virtual const char *where() const throw();

protected:
    std::string m_info;

private:
    std::string m_what;
    std::string m_where;
};

class V_UTILEXPORT not_implemented: public exception {
public:
    not_implemented(const std::string &what = "not implemented");
};

class V_UTILEXPORT assertion_failure: public exception {
public:
    assertion_failure(const std::string &what = "assertion failure");
};

class V_UTILEXPORT index_overflow: public exception {
public:
    index_overflow(const std::string &what = "index overflow");
};

class V_UTILEXPORT consistency_error: public exception {
public:
    consistency_error(const std::string &what = "consistency check failure");
};

class V_UTILEXPORT parent_died: public exception {
public:
    parent_died(const std::string &what = "parent process died");
};

} // namespace except

using except::exception;

} // namespace vistle
#endif
