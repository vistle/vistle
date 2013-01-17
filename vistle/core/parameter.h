#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include "vector.h"
#include "vistle.h"

namespace vistle {

class VCEXPORT Parameter {

 public:
   Parameter(const std::string & name);
   virtual ~Parameter();

   const std::string & getName();

 private:
   std::string name;
};

template<typename T>
class VCEXPORT ParameterBase: public Parameter {

 public:
   ParameterBase(const std::string & name, T value) : Parameter(name), value(value) {}
   virtual ~ParameterBase() {}

   const std::string & getName() { return name; }

   const T getValue() const { return value; }
   void setValue(T value) { this->value = value; }

 private:
   std::string name;
   T value;
};

typedef ParameterBase<Scalar> FloatParameter;
typedef ParameterBase<int> IntParameter;
typedef ParameterBase<Vector> VectorParameter;
typedef ParameterBase<std::string> FileParameter;

} // namespace vistle
#endif
