#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <boost/mpl/vector.hpp>
#include "paramvector.h"
#include "export.h"

namespace vistle {

typedef boost::mpl::vector<int, double, ParamVector, std::string> Parameters;

class V_COREEXPORT Parameter {

 public:
   enum Type {
      Unknown, // keep first
      Scalar,
      Integer,
      Vector,
      String,
      Invalid // keep last
   };

   enum Presentation {
      Generic, // default, keep first
      Filename, // String
      Directory, // String
      Pathname, // String
      Boolean, // Integer
      Choice, // Integer (fixed choice) and String (dynamic choice)
      InvalidPresentation // keep last
   };

   Parameter(const std::string & name, Type = Invalid, Presentation = Generic);
   virtual ~Parameter();

   void setDescription(const std::string &description);

   virtual operator std::string() const = 0;
   virtual bool isDefault() const = 0;
   const std::string & getName() const;
   Type type() const;
   Presentation presentation() const;
   const std::string &description() const;

 protected:
   std::vector<std::string> m_choices;
 private:
   std::string m_name;
   std::string m_description;
   enum Type m_type;
   enum Presentation m_presentation;
};

template<typename T>
struct ParameterType {
};

template<typename T>
struct ParameterCheck {
   static bool check(const std::vector<std::string> &, const T &) { return true; }
};

template<typename T>
class V_COREEXPORT ParameterBase: public Parameter {

 public:
   typedef T ValueType;

   ParameterBase(const std::string & name, T value = T()) : Parameter(name, ParameterType<T>::type), m_value(value), m_defaultValue(value) {}
   virtual ~ParameterBase() {}

   bool isDefault() const { return m_value == m_defaultValue; }
   const T getValue() const { return m_value; }
   virtual bool setValue(T value, bool init=false) {
      if (init)
         m_defaultValue=value;
      else if (!checkValue(value))
         return false;
      this->m_value = value;
      return true;
   }
   virtual bool checkValue(const T &value) const {
      if (presentation() != Choice) return true;
      return ParameterCheck<T>::check(m_choices, value);
   }

   operator std::string() const { std::stringstream str; str << m_value; return str.str(); }
 private:
   T m_value;
   T m_defaultValue;
};

template<>
struct ParameterType<int> {
   static const Parameter::Type type = Parameter::Integer;
};

template<>
struct ParameterType<double> {
   static const Parameter::Type type = Parameter::Scalar;
};

template<>
struct ParameterType<ParamVector> {
   static const Parameter::Type type = Parameter::Vector;
};

template<>
struct ParameterType<std::string> {
   static const Parameter::Type type = Parameter::String;
};

template<>
struct ParameterCheck<int> {
   static bool check(const std::vector<std::string> &choices, const int &value) {
      if (value < 0 || value >= choices.size()) {
         std::cerr << "IntParameter: choice out of range" << std::endl;
         return false;
      }
      return true;
   }
};

template<>
struct ParameterCheck<std::string> {
   static bool check(const std::vector<std::string> &choices, const std::string &value) {
      if (std::find(choices.begin(), choices.end(), value) == choices.end()) {
         std::cerr << "StringParameter: choice not valid" << std::endl;
         return false;
      }
      return true;
   }
};

typedef ParameterBase<ParamVector> VectorParameter;
typedef ParameterBase<double> FloatParameter;
typedef ParameterBase<int> IntParameter;
typedef ParameterBase<std::string> StringParameter;

} // namespace vistle
#endif
