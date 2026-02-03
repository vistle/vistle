#ifndef VISTLE_INSITU_CATALYST2_CONDUITTOVISTLE_H
#define VISTLE_INSITU_CATALYST2_CONDUITTOVISTLE_H

#include <catalyst_conduit.hpp>
#include <conduit.hpp>
#include <vistle/core/database.h>
#include <vistle/core/object.h>

vistle::Object::ptr conduitMeshToVistle(const conduit_cpp::Node &mesh, int sourceId);
vistle::DataBase::ptr conduitDataToVistle(const conduit_cpp::Node &field, int sourceId);

#endif
