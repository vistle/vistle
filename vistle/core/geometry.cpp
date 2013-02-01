#include "geometry.h"

namespace vistle {

Geometry::Geometry(Object::const_ptr grid, const Meta &meta)
   : Geometry::Base(Geometry::Data::create(meta))
{
   setGeometry(grid);
}

Geometry::Data::Data(const Geometry::Data &o, const std::string &n)
: Geometry::Base::Data(o, n)
, geometry(o.geometry)
, colors(o.colors)
, normals(o.normals)
, texture(o.texture)
{
}

Geometry::Data::Data(const std::string & name,
      const Meta &m)
   : Geometry::Base::Data(Object::GEOMETRY, name, meta)
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

Geometry::Data * Geometry::Data::create(const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *g = shm<Data>::construct(name)(name, meta);
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

V_OBJECT_TYPE(Geometry, Object::GEOMETRY);

} // namespace vistle
