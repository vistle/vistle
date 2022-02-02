//-------------------------------------------------------------------------
// STRUCTURED GRID CLASS IMPL H
// *
// * Structured Grid Container Object
//-------------------------------------------------------------------------
#ifndef VISTLE_LAYER_GRID_IMPL_H
#define VISTLE_LAYER_GRID_IMPL_H

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void LayerGrid::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_layerGridBase", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "num_divisions", numDivisions);
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

    ar &V_NAME(ar, "normals", normals);
}

} // namespace vistle

#endif /* STRUCTURED_GRID_IMPL_H */
