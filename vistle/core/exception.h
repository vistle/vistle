#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

#include "export.h"

namespace vistle {

namespace except {

class V_COREEXPORT exception: public std::exception {

   public:
   exception(const std::string &what = "vistle error");
   virtual ~exception() throw();

   virtual const char* what() const throw();
   virtual const char* info() const throw();
   virtual const char* where() const throw();

   private:
   std::string m_what;
   std::string m_info;
   std::string m_where;
};

class V_COREEXPORT not_implemented: public exception {

   public:
   not_implemented(const std::string &what = "not implemented");
};

class V_COREEXPORT assertion_failure: public exception {

 public:
   assertion_failure(const std::string &what = "assertion failure");
};

class V_COREEXPORT consistency_error: public exception {

 public:
   consistency_error(const std::string &what = "consistency check failure");
};

} // namespace exception

using except::exception;

} // namespace vistle
#endif
