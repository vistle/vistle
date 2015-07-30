#undef NDEBUG

#include "VistleGeometryGenerator.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture1D>
#include <osg/Point>

#include <core/polygons.h>
#include <core/points.h>
#include <core/spheres.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/texture1d.h>
#include <core/placeholder.h>
#include <core/normals.h>

#ifdef COVER_PLUGIN
#include <cover/RenderObject.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRShader.h>
#include <PluginUtil/coSphere.h>
#endif

using namespace vistle;

std::mutex VistleGeometryGenerator::s_coverMutex;

VistleGeometryGenerator::VistleGeometryGenerator(vistle::Object::const_ptr geo,
            vistle::Object::const_ptr color,
            vistle::Object::const_ptr normal,
            vistle::Object::const_ptr tex)
: m_geo(geo)
, m_color(color)
, m_normal(normal)
, m_tex(tex)
{
}

bool VistleGeometryGenerator::isSupported(vistle::Object::Type t) {

   switch (t) {
      case vistle::Object::POINTS:
      case vistle::Object::LINES:
      case vistle::Object::TRIANGLES:
      case vistle::Object::POLYGONS:
#ifdef COVER_PLUGIN
      case vistle::Object::SPHERES:
#endif
         return true;

      default:
         return false;
   }
}

osg::Node *VistleGeometryGenerator::operator()(osg::ref_ptr<osg::StateSet> defaultState) {

   if (!m_geo)
      return NULL;

   std::stringstream debug;
   debug << "[";
   debug << (m_geo ? "G" : ".");
   debug << (m_color ? "C" : ".");
   debug << (m_normal ? "N" : ".");
   debug << (m_tex ? "T" : ".");
   debug << "] ";

   int t=m_geo->getTimestep();
   if (t<0 && m_color)
      t = m_color->getTimestep();
   if (t<0 && m_normal)
      t = m_normal->getTimestep();
   if (t<0 && m_tex)
      t = m_tex->getTimestep();

   int b=m_geo->getBlock();
   if (b<0 && m_color)
      b = m_color->getBlock();
   if (b<0 && m_normal)
      b = m_normal->getBlock();
   if (b<0 && m_tex)
      b = m_tex->getBlock();

   debug << "b " << b << ", t " << t << "  ";

   osg::Geode *geode = NULL;
   osg::ref_ptr<osg::Geometry> geom;
   osg::ref_ptr<osg::StateSet> state;
   if (defaultState) {
      state = new osg::StateSet(*defaultState);
   } else {
#ifdef COVER_PLUGIN
      state = opencover::VRSceneGraph::instance()->loadDefaultGeostate();
#else
      state = new osg::StateSet;
#endif
   }

   switch (m_geo->getType()) {

      case vistle::Object::PLACEHOLDER: {

         vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
         debug << "Placeholder [" << ph->originalName() << "]";
         if (isSupported(ph->originalType())) {
            osg::Node *node = new osg::Node();
            node->setName(ph->originalName());
            std::cerr << debug.str() << std::endl;
            return node;
         }
         break;
      }

      case vistle::Object::POINTS: {

         vistle::Points::const_ptr points = vistle::Points::as(m_geo);
         const Index numVertices = points->getNumPoints();

         debug << "Points: [ #v " << numVertices << " ]";

         geode = new osg::Geode();
         geom = new osg::Geometry();

         const vistle::Scalar *x = &points->x()[0];
         const vistle::Scalar *y = &points->y()[0];
         const vistle::Scalar *z = &points->z()[0];
         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         for (Index v = 0; v < numVertices; v ++)
            vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

         geom->setVertexArray(vertices.get());
         osg::ref_ptr<osg::DrawElementsUInt> idx = new osg::DrawElementsUInt(osg::PrimitiveSet::POINTS, 0);
         for (Index p=0; p<numVertices; ++p)
            idx->push_back(p);

         geom->addPrimitiveSet(idx.get());

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
         state->setAttribute(new osg::Point(2.0f), osg::StateAttribute::ON);

         geode->addDrawable(geom.get());
         break;
      }

#ifdef COVER_PLUGIN
      case vistle::Object::SPHERES: {
         vistle::Spheres::const_ptr spheres = vistle::Spheres::as(m_geo);
         const Index numVertices = spheres->getNumSpheres();

         debug << "Spheres: [ #v " << numVertices << " ]";

         geode = new osg::Geode();
         geom = new osg::Geometry();

         const vistle::Scalar *x = &spheres->x()[0];
         const vistle::Scalar *y = &spheres->y()[0];
         const vistle::Scalar *z = &spheres->z()[0];
         const vistle::Scalar *r = &spheres->r()[0];
         opencover::coSphere *sphere = new opencover::coSphere();
         sphere->setRenderMethod(opencover::coSphere::RENDER_METHOD_CPU_BILLBOARDS);
         sphere->setCoords(numVertices, x, y, z, r);
         sphere->setColorBinding(opencover::Bind::OverAll);
         float rgba[] = { 1., 0., 0., 1. };
         sphere->updateColors(&rgba[0], &rgba[1], &rgba[2], &rgba[3]);

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
         sphere->setStateSet(state.get());

         geode->addDrawable(sphere);
         break;
      }
#endif

      case vistle::Object::TRIANGLES: {

         vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

         const Index numCorners = triangles->getNumCorners();
         const Index numVertices = triangles->getNumVertices();

         debug << "Triangles: [ #c " << numCorners << ", #v " << numVertices << " ]";

         const Index *cl = &triangles->cl()[0];
         const vistle::Scalar *x = &triangles->x()[0];
         const vistle::Scalar *y = &triangles->y()[0];
         const vistle::Scalar *z = &triangles->z()[0];

         geode = new osg::Geode();
         geom = new osg::Geometry();

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         for (Index v = 0; v < numVertices; v ++)
            vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

         geom->setVertexArray(vertices.get());

         osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
         if (numCorners > 0) {
            for (Index corner = 0; corner < numCorners; corner ++)
               corners->push_back(cl[corner]);
         } else {
            for (Index corner = 0; corner < numVertices; corner ++)
               corners->push_back(corner);
         }

         osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();
         if (m_normal) {
            vistle::Normals::const_ptr vec = vistle::Normals::as(m_normal);
            auto x = &vec->x()[0], y = &vec->y()[0], z = &vec->z()[0];
            for (Index vertex = 0; vertex < numVertices; vertex ++) {
               osg::Vec3 n(x[vertex], y[vertex], z[vertex]);
               n.normalize();
               norm->push_back(n);
            }

         } else {
            std::vector<osg::Vec3> *vertexNormals = new std::vector<osg::Vec3>[numVertices];
            if (numCorners > 0) {
               for (Index c = 0; c < numCorners; c += 3) {
                  osg::Vec3 u(x[cl[c + 0]], y[cl[c + 0]], z[cl[c + 0]]);
                  osg::Vec3 v(x[cl[c + 1]], y[cl[c + 1]], z[cl[c + 1]]);
                  osg::Vec3 w(x[cl[c + 2]], y[cl[c + 2]], z[cl[c + 2]]);
                  osg::Vec3 normal = (w - u) ^ (v - u) * -1;
                  normal.normalize();
                  vertexNormals[cl[c]].push_back(normal);
                  vertexNormals[cl[c + 1]].push_back(normal);
                  vertexNormals[cl[c + 2]].push_back(normal);
               }
            } else {
               for (Index c = 0; c < numVertices; c += 3) {
                  osg::Vec3 u(x[c + 0], y[c + 0], z[c + 0]);
                  osg::Vec3 v(x[c + 1], y[c + 1], z[c + 1]);
                  osg::Vec3 w(x[c + 2], y[c + 2], z[c + 2]);
                  osg::Vec3 normal = (w - u) ^ (v - u) * -1;
                  normal.normalize();
                  vertexNormals[c].push_back(normal);
                  vertexNormals[c + 1].push_back(normal);
                  vertexNormals[c + 2].push_back(normal);
               }
            }

            for (Index vertex = 0; vertex < numVertices; vertex ++) {
               osg::Vec3 n;
               std::vector<osg::Vec3>::iterator i;
               for (i = vertexNormals[vertex].begin(); i != vertexNormals[vertex].end(); i ++)
                  n += *i;
               norm->push_back(n);
            }
            delete[] vertexNormals;
         }

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

         geom->addPrimitiveSet(corners.get());
         geom->setNormalArray(norm.get());
         geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

         geode->addDrawable(geom.get());
         break;
      }

      case vistle::Object::LINES: {

         vistle::Lines::const_ptr lines = vistle::Lines::as(m_geo);
         const Index numElements = lines->getNumElements();
         const Index numCorners = lines->getNumCorners();

         debug << "Lines: [ #c " << numCorners << ", #e " << numElements << " ]";

         const Index *el = &lines->el()[0];
         const Index *cl = &lines->cl()[0];
         const vistle::Scalar *x = &lines->x()[0];
         const vistle::Scalar *y = &lines->y()[0];
         const vistle::Scalar *z = &lines->z()[0];

         geode = new osg::Geode();
         geom = new osg::Geometry();
         osg::ref_ptr<osg::DrawArrayLengths> primitives =
            new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

         for (Index index = 0; index < numElements; index ++) {

            Index start = el[index];
            Index end = index==numElements-1 ? numCorners : el[index+1];
            Index num = end - start;

            primitives->push_back(num);

            for (Index n = 0; n < num; n ++) {
               Index v = cl[start + n];
               vertices->push_back(osg::Vec3(x[v], y[v], z[v]));
            }
         }

         state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

         geom->setVertexArray(vertices.get());
         geom->addPrimitiveSet(primitives.get());

         geode->addDrawable(geom.get());
         break;
      }

      case vistle::Object::POLYGONS: {

         vistle::Polygons::const_ptr polygons = vistle::Polygons::as(m_geo);

         vistle::Normals::const_ptr vec;
         if (m_normal)
            vec = vistle::Normals::as(m_normal);

         const Index numElements = polygons->getNumElements();
         const Index numCorners = polygons->getNumCorners();
         const Index numVertices = polygons->getNumVertices();
         const Index numNormals = vec ? vec->getSize() : 0;

         debug << "Polygons: [ #c " << numCorners << ", #e " << numElements << ", #v " << numVertices << " ]";

         const Index *el = &polygons->el()[0];
         const Index *cl = &polygons->cl()[0];
         const vistle::Scalar *x = &polygons->x()[0];
         const vistle::Scalar *y = &polygons->y()[0];
         const vistle::Scalar *z = &polygons->z()[0];
         const vistle::Scalar *nx = NULL;
         const vistle::Scalar *ny = NULL;
         const vistle::Scalar *nz = NULL;
         if (numNormals) {
            nx = &vec->x()[0];
            ny = &vec->y()[0];
            nz = &vec->z()[0];
         }

         geode = new osg::Geode();
         geom = new osg::Geometry();
         osg::ref_ptr<osg::DrawArrayLengths> primitives =
            new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

         std::map<Index, Index> vertexMap;
         std::vector<std::vector<osg::Vec3> > vertexNormals;
         vertexNormals.resize(numVertices);

         for (Index index = 0; index < numElements; index ++) {

            const Index num = el[index + 1] - el[index];
            primitives->push_back(num);

            osg::Vec3 vert[3];
            for (Index n = 0; n < num; n ++) {
               Index v = cl[el[index] + n];

               vertexMap[vertices->size()] = v;
               osg::Vec3 vi(x[v], y[v], z[v]);
               vertices->push_back(vi);


               if (numNormals) {
                  osg::Vec3 no(nx[v], ny[v], nz[v]);
                  no.normalize();
                  norm->push_back(no);
               } else if (n < 3)
                  vert[n] = vi;
            }

            if (!numNormals) {
               osg::Vec3 no =
                  (vert[2] - vert[0]) ^ (vert[1] - vert[0]) * -1;
               no.normalize();
               for (Index n = 0; n < num; n ++)
                  vertexNormals[cl[el[index] + n]].push_back(no);
            }
         }

         // convert per face normals to per vertex normals
         for (Index vertex = 0; vertex < vertices->size(); vertex ++) {
            osg::Vec3 n;
            std::vector<osg::Vec3>::iterator i;
            for (i = vertexNormals[vertexMap[vertex]].begin();
                  i != vertexNormals[vertexMap[vertex]].end(); i++)
               n += *i;
            n.normalize();
            norm->push_back(n);
         }

         geom->setVertexArray(vertices.get());
         geom->addPrimitiveSet(primitives.get());

         geom->setNormalArray(norm.get());
         geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

         geode->addDrawable(geom.get());
         break;
      }

      default:
         assert(isSupported(m_geo->getType()) == false);
         break;
   }

   if (geode) {
      assert(m_geo->getType() == vistle::Object::PLACEHOLDER || isSupported(m_geo->getType()) == true);
      geode->setName(m_geo->getName());

      std::map<std::string, std::string> parammap;
      std::string name = m_geo->getAttribute("shader");
      std::string params = m_geo->getAttribute("shader_params");
      // format has to be '"key=value" "key=value1 value2"'
      bool escaped = false;
      std::string::size_type keyvaluestart = std::string::npos;
      for (std::string::size_type i=0; i<params.length(); ++i) {
         if (!escaped) {
            if (params[i] == '\\') {
               escaped = true;
               continue;
            }
            if (params[i] == '"') {
               if (keyvaluestart == std::string::npos) {
                  keyvaluestart = i+1;
               } else {
                  std::string keyvalue = params.substr(keyvaluestart, i-keyvaluestart-1);
                  std::string::size_type eq = keyvalue.find('=');
                  if (eq == std::string::npos) {
                     std::cerr << "ignoring " << keyvalue << ": no '=' sign" << std::endl;
                  } else {
                     std::string key = keyvalue.substr(0, eq);
                     std::string value = keyvalue.substr(eq+1);
                     //std::cerr << "found key: " << key << ", value: " << value << std::endl;
                     parammap.insert(std::make_pair(key, value));
                  }
               }
            }
         }
         escaped = false;
      }
#ifdef COVER_PLUGIN
      if (!name.empty()) {
         s_coverMutex.lock();
         opencover::coVRShader *shader = opencover::coVRShaderList::instance()->get(name, &parammap);
         shader->apply(geode);
         s_coverMutex.unlock();
      }
#endif

      if (geom)
         geom->setStateSet(state.get());

      vistle::Coords::const_ptr coords = vistle::Coords::as(m_geo);
      vistle::Indexed::const_ptr indexed = vistle::Indexed::as(m_geo);
      if (geom && coords) {
         if(vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_tex)) {

            if (tex->getNumCoords() != coords->getNumCoords()) {
               std::cerr << "VistleGeometryGenerator: Coords: texture size mismatch, expected: " << coords->getNumCoords() << ", have: " << tex->getNumCoords() << std::endl;
            } else {
               osg::ref_ptr<osg::Texture1D> osgTex = new osg::Texture1D;
               osgTex->setDataVariance(osg::Object::DYNAMIC);

               osg::ref_ptr<osg::Image> image = new osg::Image();
               image->setImage(tex->getWidth(), 1, 1, GL_RGBA, GL_RGBA,
                     GL_UNSIGNED_BYTE, &tex->pixels()[0],
                     osg::Image::NO_DELETE);
               osgTex->setImage(image);

               osg::ref_ptr<osg::FloatArray> tc = new osg::FloatArray();
               const Index *cl = (indexed && indexed->getNumCorners() > 0) ? &indexed->cl()[0] : nullptr;
               if (cl) {
                  const auto el = &indexed->el()[0];
                  const auto numElements = indexed->getNumElements();
                  for (Index index = 0; index < numElements; ++index) {
                     const Index num = el[index + 1] - el[index];
                     for (Index n = 0; n < num; n ++) {
                        Index v = cl[el[index] + n];
                        tc->push_back(tex->coords()[v]);
                     }
                  }
               } else {
                  const auto numCoords = coords->getNumCoords();
                  for (Index index = 0; index < numCoords; ++index) {
                     tc->push_back(tex->coords()[index]);
                  }
               }

               geom->setTexCoordArray(0, tc);
               state->setTextureAttributeAndModes(0, osgTex,
                     osg::StateAttribute::ON);
               osgTex->setFilter(osg::Texture1D::MIN_FILTER,
                     osg::Texture1D::NEAREST);
               osgTex->setFilter(osg::Texture1D::MAG_FILTER,
                     osg::Texture1D::NEAREST);
            }
         }

      }
   }

   //std::cerr << debug.str() << std::endl;

   return geode;
}
