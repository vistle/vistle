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

} // namespace vistle
