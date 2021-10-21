#ifndef VERTEXOWNERLIST_IMPL_H
#define VERTEXOWNERLIST_IMPL_H

namespace vistle {

template<class Archive>
void VertexOwnerList::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "vertex_list", vertexList);
    ar &V_NAME(ar, "cell_list", cellList);
}

} //namespace vistle

#endif
