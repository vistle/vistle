#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include <sstream>
#include <iostream>
#include "paramvector.h"
#include "export.h"

namespace vistle {

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
class V_COREEXPORT ParameterBase: public Parameter {

 public:
   typedef T ValueType;

   ParameterBase(const std::string & name, T value = T()) : Parameter(name, ParameterType<T>::type), m_value(value), m_defaultValue(value) {}
   virtual ~ParameterBase() {}

   const bool isDefault() const { return m_value == m_defaultValue; }
   const T getValue() const { return m_value; }
   virtual void setValue(T value, bool init=false) { this->m_value = value; if (init) m_defaultValue=value; }

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

typedef ParameterBase<ParamVector> VectorParameter;
typedef ParameterBase<double> FloatParameter;
class IntParameter: public ParameterBase<int> {
 public:
   IntParameter(const std::string & name, int value = 0) : ParameterBase<int>(name, value) {}
   void setValue(int value, bool init=false) {
      if (presentation() == Choice && !init) {
         if (value < 0 || value >= m_choices.size()) {
            std::cerr << "IntParameter: choice out of range" << std::endl;
            return;
         }
      }
      ParameterBase<int>::setValue(value, init);
   }
};
class StringParameter: public ParameterBase<std::string> {
 public:
   StringParameter(const std::string & name, std::string value = "") : ParameterBase<std::string>(name, value) {}
   void setValue(std::string value, bool init=false) {
      if (presentation() == Choice && !init) {
         if (std::find(m_choices.begin(), m_choices.end(), value) == m_choices.end()) {
            std::cerr << "StringParameter: choice not valid" << std::endl;
            return;
         }
      }
      ParameterBase<std::string>::setValue(value, init);
   }
};

} // namespace vistle
#endif
