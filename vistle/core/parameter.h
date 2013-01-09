#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include "vector.h"

namespace vistle {

class Parameter {

 public:
   enum Type {
      Unknown, // keep first
      Scalar,
      Integer,
      Vector,
      String,
      Invalid // keep last
   };
   Parameter(const std::string & name, Type = Invalid);
   virtual ~Parameter();

   const std::string & getName() const;
   Type type() const;

 private:
   std::string m_name;
   enum Type m_type;
};

template<typename T>
struct ParameterType {
};

template<typename T>
class ParameterBase: public Parameter {

 public:
   typedef T ValueType;

   ParameterBase(const std::string & name, T value = T()) : Parameter(name, ParameterType<T>::type), value(value) {}
   virtual ~ParameterBase() {}

   const T getValue() const { return value; }
   void setValue(T value) { this->value = value; }

 private:
   T value;
};

template<>
struct ParameterType<int> {
   static const Parameter::Type type = Parameter::Integer;
};

template<>
struct ParameterType<Scalar> {
   static const Parameter::Type type = Parameter::Scalar;
};

template<>
struct ParameterType<Vector> {
   static const Parameter::Type type = Parameter::Vector;
};

template<>
struct ParameterType<std::string> {
   static const Parameter::Type type = Parameter::String;
};

typedef ParameterBase<Scalar> FloatParameter;
typedef ParameterBase<int> IntParameter;
typedef ParameterBase<Vector> VectorParameter;
typedef ParameterBase<std::string> StringParameter;

} // namespace vistle
#endif
