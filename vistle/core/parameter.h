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

class FloatParameter: public Parameter {

 public:
   FloatParameter(const std::string & name, const float value);

   float getValue() const;
   void setValue(const float value);

 private:
   float value;
};

} // namespace vistle
#endif
