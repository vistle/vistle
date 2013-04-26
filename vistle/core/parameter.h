#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include <sstream>
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
      Choice, // Integer
      InvalidPresentation // keep last
   };

   Parameter(const std::string & name, Type = Invalid, Presentation = Generic);
   virtual ~Parameter();

   virtual operator std::string() const = 0;
   const std::string & getName() const;
   Type type() const;
   Presentation presentation() const;

 private:
   std::string m_name;
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

   ParameterBase(const std::string & name, T value = T()) : Parameter(name, ParameterType<T>::type), value(value) {}
   virtual ~ParameterBase() {}

   const T getValue() const { return value; }
   void setValue(T value) { this->value = value; }

   operator std::string() const { std::stringstream str; str << value; return str.str(); }
 private:
   T value;
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

typedef ParameterBase<double> FloatParameter;
typedef ParameterBase<int> IntParameter;
typedef ParameterBase<ParamVector> VectorParameter;
typedef ParameterBase<std::string> StringParameter;

} // namespace vistle
#endif
