#include "parameter.h"

namespace vistle {

Parameter::Parameter(const std::string & n)
   : name(n) {

}

Parameter::~Parameter() {

}

const std::string & Parameter::getName() {

   return name;
}

FileParameter::FileParameter(const std::string & name,
                             const std::string & v)
   : Parameter(name), value(v) {

}

const std::string & FileParameter::getValue() const {

   return value;
}

void FileParameter::setValue(const std::string & v) {

   value = v;
}

FloatParameter::FloatParameter(const std::string & name,
                               const float v)
   : Parameter(name), value(v) {

}

float FloatParameter::getValue() const {

   return value;
}

void FloatParameter::setValue(const float v) {

   value = v;
}

IntParameter::IntParameter(const std::string & name,
                           const int v)
   : Parameter(name), value(v) {

}

int IntParameter::getValue() const {

   return value;
}

void IntParameter::setValue(const int v) {

   value = v;
}

VectorParameter::VectorParameter(const std::string & name,
                                 const Vector & v)
   : Parameter(name), value(v) {

}

Vector VectorParameter::getValue() const {

   return value;
}

void VectorParameter::setValue(const Vector & v) {

   value = v;
}

} // namespace vistle
