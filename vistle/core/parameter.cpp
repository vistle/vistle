#include "parameter.h"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>

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

void Parameter::setChoices(const std::vector<std::string> &c) {

   if (checkChoice(c))
      m_choices = c;
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

namespace {

using namespace boost;

struct instantiator {
   template<typename P> P operator()(P) {
      return P();
   }
};
}

void instantiate_parameters() {

   mpl::for_each<Parameters>(instantiator());
}

} // namespace vistle
