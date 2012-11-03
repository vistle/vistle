#include "unstr.h"

namespace vistle {

UnstructuredGrid::UnstructuredGrid(const size_t numElements,
      const size_t numCorners,
      const size_t numVertices,
      const int block, const int timestep)
   : UnstructuredGrid::Base(UnstructuredGrid::Data::create(numElements, numCorners, numVertices, block, timestep))
{
}

UnstructuredGrid::Data::Data(const size_t numElements,
                                   const size_t numCorners,
                                   const size_t numVertices,
                                   const std::string & name,
                                   const int block, const int timestep)
   : UnstructuredGrid::Base::Data(numElements, numCorners, numVertices,
         Object::UNSTRUCTUREDGRID, name, block, timestep)
   , tl(new ShmVector<char>(numElements))
{
}

UnstructuredGrid::Data * UnstructuredGrid::Data::create(const size_t numElements,
                                            const size_t numCorners,
                                            const size_t numVertices,
                                            const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *u = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, block, timestep);
   publish(u);

   return u;
}

V_OBJECT_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);

} // namespace vistle
