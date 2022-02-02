//-------------------------------------------------------------------------
// UNIFORM GRID CLASS IMPL H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------
#ifndef UNIFORM_GRID_IMPL_H
#define UNIFORM_GRID_IMPL_H

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

#endif /* UNIFORM_GRID_IMPL_H */
