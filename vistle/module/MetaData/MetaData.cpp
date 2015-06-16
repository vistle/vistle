#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/triangles.h>
#include <core/unstr.h>
#include <core/vec.h>

#include "MetaData.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MetaAttribute,
      (MpiRank)
      (BlockNumber)
      (TimestepNumber)
      (VertexIndex)
)

using namespace vistle;

MODULE_MAIN(MetaData)

MetaData::MetaData(const std::string &shmname, const std::string &name, int moduleID)
   : Module("MetaData", shmname, name, moduleID) {

   createInputPort("grid_in");

   createOutputPort("data_out");

   m_kind = addIntParameter("attribute", "attribute to map to vertices", (Integer)BlockNumber, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_kind, MetaAttribute);
   m_range = addIntVectorParameter("range", "range to which data shall be clamped", 0);
   m_modulus = addIntParameter("modulus", "wrap around output value", -1);
}

MetaData::~MetaData() {
}

bool MetaData::compute() {

   auto in = expect<DataBase>("grid_in");
   if (!in)
      return true;

   Index N = in->getSize();
   Vec<Index>::ptr out(new Vec<Index>(N));
   auto val = out->x().data();

   const Index kind = m_kind->getValue();
   const Index block = in->getBlock();
   const Index timestep = in->getTimestep();

   for (Index i=0; i<N; ++i) {
      switch(kind) {
         case MpiRank: val[i] = rank(); break;
         case BlockNumber: val[i] = block; break;
         case TimestepNumber: val[i] = timestep; break;
         case VertexIndex: val[i] = i; break;
         default: val[i] = 0; break;
      }
   }

   out->setMeta(in->meta());
   addObject("data_out", out);

   return true;
}
