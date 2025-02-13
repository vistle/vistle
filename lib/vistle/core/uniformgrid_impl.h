#ifndef VISTLE_CORE_UNIFORMGRID_IMPL_H
#define VISTLE_CORE_UNIFORMGRID_IMPL_H

//-------------------------------------------------------------------------
// * Uniform Grid Container Object
//-------------------------------------------------------------------------

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void UniformGrid::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "unibase", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "min", min);
    ar &V_NAME(ar, "max", max);
    ar &V_NAME(ar, "numdivisions", numDivisions);
    ar &V_NAME(ar, "indexoffset", indexOffset);
    ar &V_NAME(ar, "ghostlayers", ghostLayers);
    ar &V_NAME(ar, "normals", normals);
}

} // namespace vistle

#endif
