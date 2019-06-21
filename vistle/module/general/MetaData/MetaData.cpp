#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/triangles.h>
#include <core/geometry.h>
#include <core/vec.h>
#include <util/math.h>

#include "MetaData.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MetaAttribute,
      (MpiRank)
      (BlockNumber)
      (TimestepNumber)
      (VertexIndex)
)

using namespace vistle;

MODULE_MAIN(MetaData)

MetaData::MetaData(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("MetaData", name, moduleID, comm) {

   createInputPort("grid_in");

   createOutputPort("data_out");

   m_kind = addIntParameter("attribute", "attribute to map to vertices", (Integer)BlockNumber, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_kind, MetaAttribute);
   m_range = addIntVectorParameter("range", "range to which data shall be clamped", IntParamVector(0,std::numeric_limits<Index>::max()));
   m_modulus = addIntParameter("modulus", "wrap around output value", -1);
}

MetaData::~MetaData() {
}

bool MetaData::compute() {

   auto obj = expect<Object>("grid_in");
   if (!obj)
       return true;
   Object::const_ptr grid;
   if (obj->getInterface<GeometryInterface>())
       grid = obj;
   DataBase::const_ptr data;
   if (grid) {
       data = DataBase::as(obj);
       if (!data)
           return true;
   } else {
      data = DataBase::as(obj);
      if (!data) {
         return true;
      }
      if (data->grid() && data->grid()->getInterface<GeometryInterface>())
          grid = data->grid();
      if (!grid) {
         return true;
      }
   }

   Index N = data->getSize();
   Vec<Index>::ptr out(new Vec<Index>(N));
   auto val = out->x().data();

   const Index kind = m_kind->getValue();
   const Index block = data->getBlock();
   const Index timestep = data->getTimestep();

   const Index min = m_range->getValue()[0];
   const Index max = m_range->getValue()[1];
   const Index mod = m_modulus->getValue();
   for (Index i=0; i<N; ++i) {
      switch(kind) {
         case MpiRank: val[i] = rank(); break;
         case BlockNumber: val[i] = block; break;
         case TimestepNumber: val[i] = timestep; break;
         case VertexIndex: val[i] = i; break;
         default: val[i] = 0; break;
      }
      if (mod > 0)
          val[i] %= mod;
      val[i] = clamp(val[i], min, max);
   }

   std::string species;
   switch(kind) {
   case MpiRank: species = "rank"; break;
   case BlockNumber: species = "block"; break;
   case TimestepNumber: species = "timestep"; break;
   case VertexIndex: species = "vertex"; break;
   default: species = "zero"; break;
   }

   out->addAttribute("_species", species);
   out->setMeta(data->meta());
   out->setGrid(grid);
   addObject("data_out", out);

   return true;
}
