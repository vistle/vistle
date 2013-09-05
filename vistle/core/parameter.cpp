#include "parameter.h"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>

namespace vistle {

Parameter::Parameter(int moduleId, const std::string & n, Parameter::Type type, Parameter::Presentation p)
   : m_module(moduleId)
   , m_name(n)
   , m_type(type)
   , m_presentation(p)
{

}

Parameter::Parameter(const Parameter &other)
: m_choices(other.m_choices)
, m_module(other.m_module)
, m_name(other.m_name)
, m_description(other.m_description)
, m_type(other.m_type)
, m_presentation(other.m_presentation)
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

int Parameter::module() const {

   return m_module;
}

const std::string & Parameter::getName() const {

   return m_name;
}

Parameter::Type Parameter::type() const {

   return m_type;
}

void Parameter::setPresentation(Parameter::Presentation pres) {
   m_presentation = pres;
}

Parameter::Presentation Parameter::presentation() const {

   return m_presentation;
}

const std::vector<std::string> &Parameter::choices() const
{

   return m_choices;
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
