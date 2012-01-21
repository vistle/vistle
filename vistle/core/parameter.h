#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>

namespace vistle {

class Parameter {

 public:
   Parameter(const std::string & name);
   virtual ~Parameter();

   const std::string & getName();

 private:
   std::string name;
};

class FileParameter: public Parameter {

 public:
   FileParameter(const std::string & name, const std::string & value);

   const std::string & getValue() const;
   void setValue(const std::string & value);

 private:
   std::string value;
};

} // namespace vistle
#endif
