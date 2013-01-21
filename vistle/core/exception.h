#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>
#include "vistle.h"

namespace vistle {

namespace except {

class VCEXPORT exception: public std::exception {

   public:
   exception(const std::string &what = "vistle error");
   virtual ~exception() throw();

   virtual const char* what() const throw();

   private:
   std::string m_what;
};

class VCEXPORT not_implemented: public exception {

   public:
   not_implemented(const std::string &what = "not implemented");
};

} // namespace exception

using except::exception;

} // namespace vistle
#endif
