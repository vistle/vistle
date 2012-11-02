#include <iostream>
#include <iomanip>

#include <limits.h>

#include "message.h"
#include "messagequeue.h"
#include "shm.h"
#include "object.h"
#include "objects.h"

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

using namespace boost::interprocess;

namespace vistle {

Coords::Data::Data(const size_t numVertices,
      Type id, const std::string &name,
      const int block, const int timestep)
   : Coords::Base::Data(numVertices,
         id, name,
         block, timestep)
{
}

Coords::Info *Coords::getInfo(Coords::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numVertices = getNumVertices();

   return info;
}

size_t Coords::getNumVertices() const {

   return getSize();
}

Triangles::Triangles(const size_t numCorners, const size_t numVertices,
                     const int block, const int timestep)
   : Triangles::Base(Triangles::Data::create(numCorners, numVertices,
            block, timestep)) {
}

Triangles::Data::Data(const size_t numCorners, const size_t numVertices,
                     const std::string & name,
                     const int block, const int timestep)
   : Base::Data(numCorners,
         Object::TRIANGLES, name,
         block, timestep)
   , cl(new ShmVector<size_t>(numCorners))
{
}


Triangles::Data * Triangles::Data::create(const size_t numCorners,
                              const size_t numVertices,
                              const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *t = shm<Data>::construct(name)(numCorners, numVertices, name, block, timestep);
   publish(t);

   return t;
}

Triangles::Info *Triangles::getInfo(Triangles::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numCorners = getNumCorners();
   info->numVertices = getNumVertices();

   return info;
}

size_t Triangles::getNumCorners() const {

   return d()->cl->size();
}

size_t Triangles::getNumVertices() const {

   return d()->x->size();
}

Indexed::Info *Indexed::getInfo(Indexed::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numVertices = getNumVertices();
   info->numElements = getNumElements();
   info->numCorners = getNumCorners();

   return info;
}

Indexed::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices,
             Type id, const std::string & name,
             const int block, const int timestep)
   : Indexed::Base::Data(numVertices, id, name,
         block, timestep)
   , el(new ShmVector<size_t>(numElements))
   , cl(new ShmVector<size_t>(numCorners))
{
}

Lines::Lines(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const int block, const int timestep)
   : Lines::Base(Lines::Data::create(numElements, numCorners,
            numVertices, block, timestep))
{
}

Lines::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices, const std::string & name,
             const int block, const int timestep)
   : Lines::Base::Data(numElements, numCorners, numVertices,
         Object::LINES, name, block, timestep)
{
}


Lines::Data * Lines::Data::create(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, block, timestep);
   publish(l);

   return l;
}

Lines::Info *Lines::getInfo(Lines::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize += info->numElements * sizeof(uint64_t)
      + info->numCorners * sizeof(uint64_t)
      + 3 * info->numVertices * sizeof(Scalar);
   info->offset = 0;
   info->numElements = getNumElements();
   info->numCorners = getNumCorners();
   info->numVertices = getNumVertices();

   return info;
}

size_t Indexed::getNumElements() const {

   return d()->el->size();
}

size_t Indexed::getNumCorners() const {

   return d()->cl->size();
}

size_t Indexed::getNumVertices() const {

   return d()->x->size();
}

Polygons::Polygons(const size_t numElements,
      const size_t numCorners,
      const size_t numVertices,
      const int block, const int timestep)
: Polygons::Base(Polygons::Data::create(numElements, numCorners, numVertices, block, timestep))
{
}

Polygons::Data::Data(const size_t numElements, const size_t numCorners,
                   const size_t numVertices, const std::string & name,
                   const int block, const int timestep)
   : Polygons::Base::Data(numElements, numCorners, numVertices,
         Object::POLYGONS, name, block, timestep)
{
}


Polygons::Data * Polygons::Data::create(const size_t numElements,
                            const size_t numCorners,
                            const size_t numVertices,
                            const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *p = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, block, timestep);
   publish(p);

   return p;
}

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

Set::Set(const size_t numElements,
                  const int block, const int timestep)
: Set::Base(Set::Data::create(numElements, block, timestep))
{
}

Set::Data::Data(const size_t numElements, const std::string & name,
         const int block, const int timestep)
   : Set::Base::Data(Object::SET, name, block, timestep)
     , elements(new ShmVector<offset_ptr<Object::Data> >(numElements))
{
}

Set::Data::~Data() {

   for (size_t i = 0; i < elements->size(); ++i) {

      if ((*elements)[i]) {
         (*elements)[i]->unref();
      }
   }
}


Set::Data * Set::Data::Data::create(const size_t numElements,
                  const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *p = shm<Data>::construct(name)(numElements, name, block, timestep);
   publish(p);

   return p;
}

size_t Set::getNumElements() const {

   return d()->elements->size();
}

Object::const_ptr Set::getElement(const size_t index) const {

   if (index >= d()->elements->size())
      return Object::ptr();

   return Object::create((*d()->elements)[index].get());
}

void Set::setElement(const size_t index, Object::const_ptr object) {

   if (index >= d()->elements->size()) {

      std::cerr << "WARNING: automatically resisizing set" << std::endl;
      d()->elements->resize(index+1);
   }

   if (Object::const_ptr old = getElement(index)) {
      old->unref();
   }

   if (object) {
      object->ref();
      (*d()->elements)[index] = object->d();
   } else {
      (*d()->elements)[index] = NULL;
   }
}

void Set::addElement(Object::const_ptr object) {

   if (object) {
      object->ref();
      d()->elements->push_back(object->d());
   }
}

Geometry::Geometry(const int block, const int timestep)
   : Geometry::Base(Geometry::Data::create(block, timestep))
{
}

Geometry::Data::Data(const std::string & name,
      const int block, const int timestep)
   : Geometry::Base::Data(Object::GEOMETRY, name, block, timestep)
   , geometry(NULL)
   , colors(NULL)
   , normals(NULL)
   , texture(NULL)
{

   if (geometry)
      geometry->ref();

   if (colors)
      colors->ref();

   if (normals)
      normals->ref();

   if (texture)
      texture->ref();
}

Geometry::Data::~Data() {

   if (geometry)
      geometry->unref();

   if (colors)
      colors->unref();

   if (normals)
      normals->unref();

   if (texture)
      texture->unref();
}

Geometry::Data * Geometry::Data::create(const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *g = shm<Data>::construct(name)(name, block, timestep);
   publish(g);

   return g;
}

void Geometry::setGeometry(Object::const_ptr g) {

   if (Object::const_ptr old = geometry())
      old->unref();
   d()->geometry = g->d();
   if (g)
      g->ref();
}

void Geometry::setColors(Object::const_ptr c) {

   if (Object::const_ptr old = colors())
      old->unref();
   d()->colors = c->d();
   if (c)
      c->ref();
}

void Geometry::setTexture(Object::const_ptr t) {

   if (Object::const_ptr old = texture())
      old->unref();
   d()->texture = t->d();
   if (t)
      t->ref();
}

void Geometry::setNormals(Object::const_ptr n) {

   if (Object::const_ptr old = normals())
      old->unref();
   d()->normals = n->d();
   if (n)
      n->ref();
}

Object::const_ptr Geometry::geometry() const {

   return Object::create(&*d()->geometry);
}

Object::const_ptr Geometry::colors() const {

   return Object::create(&*d()->colors);
}

Object::const_ptr Geometry::normals() const {

   return Object::create(&*d()->normals);
}

Object::const_ptr Geometry::texture() const {

   return Object::create(&*d()->texture);
}

Texture1D::Texture1D(const size_t width,
      const Scalar min, const Scalar max,
      const int block, const int timestep)
: Texture1D::Base(Texture1D::Data::create(width, min, max, block, timestep))
{
}

Texture1D::Data::Data(const std::string &name, const size_t width,
                     const Scalar mi, const Scalar ma,
                     const int block, const int timestep)
   : Texture1D::Base::Data(Object::TEXTURE1D, name, block, timestep)
   , min(mi)
   , max(ma)
   , pixels(new ShmVector<unsigned char>(width * 4))
   , coords(new ShmVector<Scalar>(1))
{

#if  0
   const allocator<Scalar, managed_shared_memory::segment_manager>
      alloc_inst_Scalar(Shm::instance().getShm().get_segment_manager());

   coords = Shm::instance().getShm().construct<shm<Scalar>::vector>(Shm::instance().createObjectID().c_str())(alloc_inst_Scalar);
#endif
}

Texture1D::Data *Texture1D::Data::create(const size_t width,
                              const Scalar min, const Scalar max,
                              const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *tex= shm<Data>::construct(name)(name, width, min, max, block, timestep);
   publish(tex);

   return tex;
}

size_t Texture1D::getNumElements() const {

   return d()->coords->size();
}

size_t Texture1D::getWidth() const {

   return d()->pixels->size() / 4;
}


template<> const Object::Type Vec<Scalar>::s_type  = Object::VECFLOAT;
template<> const Object::Type Vec<int>::s_type    = Object::VECINT;
template<> const Object::Type Vec<char>::s_type   = Object::VECCHAR;
template<> const Object::Type Vec3<Scalar>::s_type = Object::VEC3FLOAT;
template<> const Object::Type Vec3<int>::s_type   = Object::VEC3INT;
template<> const Object::Type Vec3<char>::s_type  = Object::VEC3CHAR;

namespace {
class RegisterTypes {
   public:
   RegisterTypes() {
      ObjectTypeRegistry::registerType<Vec<char> >(Object::VECCHAR);
      ObjectTypeRegistry::registerType<Vec<int> >(Object::VECINT);
      ObjectTypeRegistry::registerType<Vec<Scalar> >(Object::VECFLOAT);
      ObjectTypeRegistry::registerType<Vec3<char> >(Object::VEC3CHAR);
      ObjectTypeRegistry::registerType<Vec3<int> >(Object::VEC3INT);
      ObjectTypeRegistry::registerType<Vec3<Scalar> >(Object::VEC3FLOAT);
      ObjectTypeRegistry::registerType<Lines>(Object::LINES);
      ObjectTypeRegistry::registerType<Triangles>(Object::TRIANGLES);
      ObjectTypeRegistry::registerType<Polygons>(Object::POLYGONS);
      ObjectTypeRegistry::registerType<UnstructuredGrid>(Object::UNSTRUCTUREDGRID);
      ObjectTypeRegistry::registerType<Texture1D>(Object::TEXTURE1D);
      ObjectTypeRegistry::registerType<Geometry>(Object::GEOMETRY);
      ObjectTypeRegistry::registerType<Set>(Object::SET);
   }
};

static RegisterTypes registerTypes;
}

} // namespace vistle
