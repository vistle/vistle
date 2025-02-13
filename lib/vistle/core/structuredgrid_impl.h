#ifndef VISTLE_CORE_STRUCTUREDGRID_IMPL_H
#define VISTLE_CORE_STRUCTUREDGRID_IMPL_H

//-------------------------------------------------------------------------
// * Structured Grid Container Object
//-------------------------------------------------------------------------

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void StructuredGrid::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "strbase", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "numdivisions", numDivisions);
    ar &V_NAME(ar, "indexoffset", indexOffset);
    ar &V_NAME(ar, "ghostlayers", ghostLayers);
    ar &V_NAME(ar, "normals", normals);
}

} // namespace vistle

#endif
