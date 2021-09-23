#ifndef VISITDATATYPESTOVISTLE_H
#define VISITDATATYPESTOVISTLE_H

#include "Exception.h"
#include <vistle/core/database.h>
#include <vistle/core/object.h>

#include <vistle/insitu/core/dataType.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

namespace vistle {
namespace insitu {
namespace libsim {
constexpr DataType dataTypeToVistle(int dataType)
{
    switch (dataType) {
    case VISIT_DATATYPE_CHAR:
        return DataType::CHAR;
        break;
    case VISIT_DATATYPE_INT:
        return DataType::INT;
        break;
    case VISIT_DATATYPE_DOUBLE:
        return DataType::DOUBLE;
        break;
    case VISIT_DATATYPE_LONG:
        return DataType::LONG;
        break;
    case VISIT_DATATYPE_FLOAT:
        return DataType::FLOAT;
        break;
    default:
        return DataType::INVALID;
        break;
    }
}
constexpr vistle::DataBase::Mapping mappingToVistle(int mapping)
{
    switch (mapping) {
    case VISIT_VARCENTERING_NODE:
        return vistle::DataBase::Vertex;
        break;
    case VISIT_VARCENTERING_ZONE:
        return vistle::DataBase::Element;
        break;
    default:
        return vistle::DataBase::Unspecified;
        break;
    }
}

constexpr vistle::Object::Type objTypeToVistle(int objType)
{
    switch (objType) {
    case VISIT_MESHTYPE_RECTILINEAR:
        return vistle::Object::Type::RECTILINEARGRID;
        break;
    case VISIT_MESHTYPE_CURVILINEAR:
        return vistle::Object::Type::STRUCTUREDGRID;
        break;
    case VISIT_MESHTYPE_UNSTRUCTURED:
        return vistle::Object::Type::UNSTRUCTUREDGRID;
        break;
    case VISIT_MESHTYPE_AMR:
        return vistle::Object::Type::RECTILINEARGRID;
        break;
    default:
        return vistle::Object::Type::UNKNOWN;
        break;
    }
}

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISITDATATYPESTOVISTLE_H
