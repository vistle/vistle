#ifndef VISTLE_ALG_OBJALG_H
#define VISTLE_ALG_OBJALG_H

#include "export.h"
#include <vistle/core/object.h>
#include <vistle/core/normals.h>
#include <vistle/core/database.h>

namespace vistle {

struct V_ALGEXPORT DataComponents {
    Object::const_ptr geometry;
    Normals::const_ptr normals;
    DataBase::const_ptr mapped;

    //DataBase::Mapping mapping = DataBase::Unspecified;

    int timestep = -1;
    int block = -1;
    int iteration = -1;
};

//! derive geometry w/ normals and mapped data from container
V_ALGEXPORT DataComponents splitContainerObject(Object::const_ptr container);

} // namespace vistle
#endif
