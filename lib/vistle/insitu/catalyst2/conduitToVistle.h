#ifndef VISTLE_INSITU_CATALYST2_CONDUITTOVISTLE_H
#define VISTLE_INSITU_CATALYST2_CONDUITTOVISTLE_H


#include <conduit.hpp>
#include <vistle/core/object.h>
#include <catalyst_conduit.hpp>
#include <vistle/core/database.h>
vistle::Object::ptr conduitMeshToVistle(const conduit_cpp::Node &mesh, int sourceId);
vistle::DataBase::ptr conduitDataToVistle(const conduit_cpp::Node &field, int sourceId);

#endif
