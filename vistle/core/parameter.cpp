#include "parameter.h"

namespace vistle {

Parameter::Parameter(const std::string & n, Parameter::Type type, Parameter::Presentation p)
   : m_name(n)
   , m_type(type)
   , m_presentation(p)
{

}

Parameter::~Parameter() {

}

void Parameter::setDescription(const std::string &d) {

   m_description = d;
}

const std::string & Parameter::getName() const {

   return m_name;
}

Parameter::Type Parameter::type() const {

   return m_type;
}

Parameter::Presentation Parameter::presentation() const {

   return m_presentation;
}

const std::string &Parameter::description() const {

   return m_description;
}

} // namespace vistle
