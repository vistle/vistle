#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <limits>
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
      Slider, // Integer, Float
      InvalidPresentation // keep last
   };

   Parameter(int moduleId, const std::string & name, Type = Invalid, Presentation = Generic);
   Parameter(const Parameter &other);
   virtual ~Parameter();

   virtual Parameter *clone() const = 0;

   void setPresentation(Presentation presentation);
   void setDescription(const std::string &description);
   void setChoices(const std::vector<std::string> &choices);

   virtual operator std::string() const = 0;
   virtual bool isDefault() const = 0;
   virtual bool checkChoice(const std::vector<std::string> &choices) const { return true; }
   int module() const;
   const std::string & getName() const;
   Type type() const;
   Presentation presentation() const;
   const std::string &description() const;

 protected:
   std::vector<std::string> m_choices;
 private:
   int m_module;
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

   ParameterBase(int moduleId, const std::string & name, T value = T())
      : Parameter(moduleId, name, ParameterType<T>::type)
      , m_value(value)
      , m_defaultValue(value)
      , m_minimum(ParameterType<T>::min())
      , m_maximum(ParameterType<T>::max())
      {}
   ParameterBase(const ParameterBase<T> &other)
   : Parameter(other)
   , m_value(other.m_value)
   , m_defaultValue(other.m_defaultValue)
   , m_minimum(other.m_minimum)
   , m_maximum(other.m_maximum)
   {}
   virtual ~ParameterBase() {}

   ParameterBase<T> *clone() const {
      return new ParameterBase<T>(*this);
   }

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
      if (ParameterType<T>::isNumber) {
         if (value < m_minimum)
            return false;
         if (value > m_maximum)
            return false;
      }
      if (presentation() != Choice) return true;
      return ParameterCheck<T>::check(m_choices, value);
   }
   virtual bool checkChoice(const std::vector<std::string> &ch) const {
      if (presentation() != Choice) return false;
      return ParameterCheck<T>::check(ch, m_value);
   }
   virtual void setMinimum(const T &value) {
      m_minimum = value;
   }
   virtual void setMaximum(const T &value) {
      m_maximum = value;
   }

   operator std::string() const { std::stringstream str; str << m_value; return str.str(); }
 private:
   T m_value;
   T m_defaultValue;
   T m_minimum, m_maximum;
};

template<>
struct ParameterType<int> {
   typedef int T;
   static const Parameter::Type type = Parameter::Integer;
   static const bool isNumber = true;
   static const T min() { return std::numeric_limits<T>::min(); }
   static const T max() { return std::numeric_limits<T>::max(); }
};

template<>
struct ParameterType<double> {
   typedef double T;
   static const Parameter::Type type = Parameter::Scalar;
   static const bool isNumber = true;
   static const T min() { return -std::numeric_limits<T>::max(); }
   static const T max() { return std::numeric_limits<T>::max(); }
};

template<>
struct ParameterType<ParamVector> {
   typedef ParamVector T;
   static const Parameter::Type type = Parameter::Vector;
   static const bool isNumber = true;
   static const T min() { return T::min(); }
   static const T max() { return T::max(); }
};

template<>
struct ParameterType<std::string> {
   typedef std::string T;
   static const Parameter::Type type = Parameter::String;
   static const bool isNumber = false;
   static const T min() { return ""; }
   static const T max() { return ""; }
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
