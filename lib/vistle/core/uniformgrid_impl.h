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
    ar &V_NAME(ar, "base_structuredGridBase", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "numElements", numDivisions);
    ar &V_NAME(ar, "min", min);
    ar &V_NAME(ar, "max", max);

    for (int c = 0; c < 3; c++) {
        std::string nvpTag = "ghostLayers" + std::to_string(c);
        std::string tag0 = nvpTag + "0";
        std::string tag1 = nvpTag + "1";
        ar &V_NAME(ar, tag0.c_str(), ghostLayers[c][0]);
        ar &V_NAME(ar, tag1.c_str(), ghostLayers[c][1]);

        ar &V_NAME(ar, ("indexoffset" + std::to_string(c)).c_str(), indexOffset[c]);
    }
}

} // namespace vistle

#endif /* UNIFORM_GRID_IMPL_H */
