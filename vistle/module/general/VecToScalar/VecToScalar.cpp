#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>

#include "VecToScalar.h"
#include <util/enum.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (X)(Y)(Z)(AbsoluteValue))

VecToScalar::VecToScalar(const std::string &shmname, const std::string &name, int moduleID)
   : Module("VecToScalar", shmname, name, moduleID) {
   createInputPort("data_in");
   createOutputPort("data_out");
   m_caseParam = addIntParameter("Choose Scalar Value", "Choose Scalar Value", 3, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_caseParam, Choices);
}

VecToScalar::~VecToScalar() {
}

bool VecToScalar::compute() {

   Object::const_ptr data = expect<Object>("data_in");
   if (!data)
      return true;

   if(auto data_in = Vec<Scalar, 3>::as(data)) {

      Object::ptr out;
      switch(m_caseParam->getValue()) {
         case X: {
            out = extract(data_in,0);
            break;
         }

         case Y: {
            out = extract(data_in,1);
            break;
         }

         case Z: {
            out = extract(data_in,2);
            break;
         }

         case AbsoluteValue: {
            out = calculateAbsolute(data_in);
            break;
         }
      }

      out->copyAttributes(data);
      addObject("data_out", out);
   } else {
      std::cerr << "VecToScalar: ERROR: Input is not vector data" << std::endl;
   }
      return true;
}

Object::ptr VecToScalar::extract(Vec<Scalar, 3>::const_ptr &data, const Index &coord) {
   Vec<Scalar>::ptr dataOut(new Vec<Scalar>(Object::Initialized));
   dataOut->d()->x[0] = data->d()->x[coord];
   return dataOut;
}

Object::ptr VecToScalar::calculateAbsolute(Vec<Scalar, 3>::const_ptr &data) {
   const Scalar *in_data_0 = &data->x()[0];
   const Scalar *in_data_1 = &data->y()[0];
   const Scalar *in_data_2 = &data->z()[0];
   Index dataSize = data->getSize();
   Vec<Scalar>::ptr dataOut(new Vec<Scalar>(dataSize));
   Scalar *out_data = dataOut->x().data();
   for (Index i = 0; i < dataSize; ++i)
   {
      out_data[i] = sqrt(in_data_0[i] * in_data_0[i] + in_data_1[i] * in_data_1[i] + in_data_2[i] * in_data_2[i]);
   }
   return dataOut;
}

MODULE_MAIN(VecToScalar)
