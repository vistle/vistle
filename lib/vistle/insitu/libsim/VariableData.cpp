#include "VariableData.h"
#include "Exception.h"

#include <vistle/core/unstr.h>
#include <vistle/insitu/message/SyncShmIDs.h>

vistle::Object::ptr vistle::insitu::libsim::VariableData::get(const visit_handle &varHandle)
{
    return vistle::Object::ptr();
}

vistle::Vec<vistle::Scalar, 1>::ptr
vistle::insitu::libsim::VariableData::allocVarForCombinedMesh(const VariableInfo &varInfo, vistle::Object::ptr mesh)
{
    auto unstr = vistle::UnstructuredGrid::as(varInfo.meshInfo.grids[0]);
    if (!unstr) {
        throw InsituException{} << "allocVarForCombinedMesh: combined grids must be UnstructuredGrids";
    }
    vistle::Index size =
        varInfo.mapping == vistle::DataBase::Mapping::Vertex ? unstr->getNumVertices() : unstr->getNumElements();
    auto var = std::make_shared<vistle::Vec<vistle::Scalar, 1>>(size);
    var->setMapping(varInfo.mapping);
    var->setGrid(unstr);

    return var;
}
