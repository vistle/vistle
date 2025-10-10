// fewer files compile faster
#include "object.h"
#include "empty.h"
#include "archives.h"
#include "vec.h"
#include "coords.h"
#include "normals.h"
#include "points.h"
#include "indexed.h"
#include "lines.h"
#include "triangles.h"
#include "quads.h"
#include "polygons.h"
#include "empty.h"
#include "placeholder.h"
#include "celltree.h"
#include "celltree_impl.h"
#include "vertexownerlist.h"
#include "unstr.h"
#include "structuredgridbase.h"
#include "uniformgrid.h"
#include "layergrid.h"
#include "rectilineargrid.h"
#include "structuredgrid.h"

namespace vistle {

#define REGISTER_TYPE(ObjType, id) \
    do { \
        ObjectTypeRegistry::registerType<ObjType>(id); \
    } while (false)

#define REGISTER_VEC_TYPE(t) \
    ObjectTypeRegistry::registerType<Vec<t, 1>>(Vec<t, 1>::type()); \
    ObjectTypeRegistry::registerType<Vec<t, 1>>(Vec<t, 2>::type()); \
    ObjectTypeRegistry::registerType<Vec<t, 3>>(Vec<t, 3>::type());

#define REGISTER_CELLTREE_TYPE(ct) ObjectTypeRegistry::registerType<ct>(ct::type());

void registerTypes()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    using namespace vistle;
    REGISTER_TYPE(Empty, Object::EMPTY);
    REGISTER_TYPE(PlaceHolder, Object::PLACEHOLDER);
    REGISTER_TYPE(Points, Object::POINTS);
    REGISTER_TYPE(Lines, Object::LINES);
    REGISTER_TYPE(Triangles, Object::TRIANGLES);
    REGISTER_TYPE(Quads, Object::QUADS);
    REGISTER_TYPE(Polygons, Object::POLYGONS);
    REGISTER_TYPE(UniformGrid, Object::UNIFORMGRID);
    REGISTER_TYPE(LayerGrid, Object::LAYERGRID);
    REGISTER_TYPE(RectilinearGrid, Object::RECTILINEARGRID);
    REGISTER_TYPE(StructuredGrid, Object::STRUCTUREDGRID);
    REGISTER_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);
    REGISTER_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST);
    REGISTER_TYPE(Normals, Object::NORMALS);

    FOR_ALL_SCALARS(REGISTER_VEC_TYPE);

    FOR_ALL_CELLTREE_TYPES(REGISTER_CELLTREE_TYPE);
}

} // namespace vistle
