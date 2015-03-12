#ifndef VERTEXOWNERLIST_IMPL_H
#define VERTEXOWNERLIST_IMPL_H

namespace vistle {

template<class Archive>
void VertexOwnerList::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("vertex_list", *vertexList);
   ar & V_NAME("cell_list", *cellList);
}

}//namespace vistle

#endif


