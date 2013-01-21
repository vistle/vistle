#include "parameter.h"

namespace vistle {

Parameter::Parameter(const std::string & n, Parameter::Type type)
   : m_name(n)
   , m_type(type) {

}

Parameter::~Parameter() {

}

const std::string & Parameter::getName() const {

   return m_name;
}

Parameter::Type Parameter::type() const {

   return m_type;
}

} // namespace vistle
