#ifndef VISTLE_CORE_RECTILINEARGRID_IMPL_H
#define VISTLE_CORE_RECTILINEARGRID_IMPL_H

//-------------------------------------------------------------------------
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void RectilinearGrid::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "rectbase", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "coords_x", coords[0]);
    ar &V_NAME(ar, "coords_y", coords[1]);
    ar &V_NAME(ar, "coords_z", coords[2]);
    ar &V_NAME(ar, "indexoffset", indexOffset);
    ar &V_NAME(ar, "ghostlayers", ghostLayers);
    ar &V_NAME(ar, "normals", normals);
}

} // namespace vistle

#endif
