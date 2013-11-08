#include "VistleGeometryGenerator.h"
#include <kernel/VRSceneGraph.h>
#include <kernel/coVRShader.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture1D>
#include <osg/Point>

#include <core/polygons.h>
#include <core/points.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/texture1d.h>
#include <core/placeholder.h>
#include <core/normals.h>

using namespace opencover;
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
      case vistle::Object::GEOMETRY:
      case vistle::Object::POINTS:
      case vistle::Object::TRIANGLES:
      case vistle::Object::LINES:
      case vistle::Object::POLYGONS:
         return true;

      default:
         return false;
   }
}

osg::Node *VistleGeometryGenerator::operator()() {

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

   switch (m_geo->getType()) {

      case vistle::Object::PLACEHOLDER: {

         vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
         if (isSupported(ph->originalType())) {
            osg::Node *node = new osg::Node();
            node->setName(ph->originalName());
            return node;
         }
         break;
      }

      case vistle::Object::POINTS: {

         vistle::Points::const_ptr points = vistle::Points::as(m_geo);
         const Index numVertices = points->getNumPoints();

         //std::cerr << debug.str() << "Points: [ #v " << numVertices << " ]" << std::endl;

         geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

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

         osg::ref_ptr<osg::StateSet> state = VRSceneGraph::instance()->loadDefaultGeostate();
         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
         state->setAttribute(new osg::Point(2.0f), osg::StateAttribute::ON);
         geom->setStateSet(state.get());

         geode->addDrawable(geom.get());
         break;
      }

      case vistle::Object::TRIANGLES: {

         vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

         const Index numCorners = triangles->getNumCorners();
         const Index numVertices = triangles->getNumVertices();

         //std::cerr << debug.str() << "Triangles: [ #c " << numCorners << ", #v " << numVertices << " ]" << std::endl;

         Index *cl = &triangles->cl()[0];
         vistle::Scalar *x = &triangles->x()[0];
         vistle::Scalar *y = &triangles->y()[0];
         vistle::Scalar *z = &triangles->z()[0];

         geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         for (Index v = 0; v < numVertices; v ++)
            vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

         geom->setVertexArray(vertices.get());

         osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
         for (Index corner = 0; corner < numCorners; corner ++)
            corners->push_back(cl[corner]);

         std::vector<osg::Vec3> *vertexNormals = new std::vector<osg::Vec3>[numVertices];
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

         osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();
         for (Index vertex = 0; vertex < numVertices; vertex ++) {
            osg::Vec3 n;
            std::vector<osg::Vec3>::iterator i;
            for (i = vertexNormals[vertex].begin(); i != vertexNormals[vertex].end(); i ++)
               n += *i;
            norm->push_back(n);
         }
         delete[] vertexNormals;

         osg::ref_ptr<osg::StateSet> state = VRSceneGraph::instance()->loadDefaultGeostate();
         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

         if(vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_tex)) {

            osg::ref_ptr<osg::Texture1D> osgTex = new osg::Texture1D;
            osgTex->setDataVariance(osg::Object::DYNAMIC);

            osg::ref_ptr<osg::Image> image = new osg::Image();

            image->setImage(tex->getWidth(), 1, 1, GL_RGBA, GL_RGBA,
                  GL_UNSIGNED_BYTE, &tex->pixels()[0],
                  osg::Image::NO_DELETE);
            osgTex->setImage(image);

            osg::ref_ptr<osg::FloatArray> coords = new osg::FloatArray();
            std::copy(tex->coords().begin(), tex->coords().end(),
                  std::back_inserter(*coords));

            geom->setTexCoordArray(0, coords);
            state->setTextureAttributeAndModes(0, osgTex,
                  osg::StateAttribute::ON);
            osgTex->setFilter(osg::Texture1D::MIN_FILTER,
                  osg::Texture1D::NEAREST);
            osgTex->setFilter(osg::Texture1D::MAG_FILTER,
                  osg::Texture1D::NEAREST);
         }

         geom->addPrimitiveSet(corners.get());
         geom->setNormalArray(norm.get());
         geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

         geode->addDrawable(geom.get());

         geom->setStateSet(state.get());

         break;
      }

      case vistle::Object::LINES: {

         vistle::Lines::const_ptr lines = vistle::Lines::as(m_geo);
         const Index numElements = lines->getNumElements();
         const Index numCorners = lines->getNumCorners();

         //std::cerr << debug.str() << "Lines: [ #c " << numCorners << ", #e " << numElements << " ]" << std::endl;

         Index *el = &lines->el()[0];
         Index *cl = &lines->cl()[0];
         vistle::Scalar *x = &lines->x()[0];
         vistle::Scalar *y = &lines->y()[0];
         vistle::Scalar *z = &lines->z()[0];

         geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
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

         //std::cerr << debug.str() << "Polygons: [ #c " << numCorners << ", #e " << numElements << ", #v " << numVertices << " ]" << std::endl;

         Index *el = &polygons->el()[0];
         Index *cl = &polygons->cl()[0];
         vistle::Scalar *x = &polygons->x()[0];
         vistle::Scalar *y = &polygons->y()[0];
         vistle::Scalar *z = &polygons->z()[0];
         vistle::Scalar *nx = NULL;
         vistle::Scalar *ny = NULL;
         vistle::Scalar *nz = NULL;
         if (numNormals) {
            nx = &vec->x()[0];
            ny = &vec->y()[0];
            nz = &vec->z()[0];
         }

         geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
         osg::ref_ptr<osg::DrawArrayLengths> primitives =
            new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

         osg::ref_ptr<osg::StateSet> state = VRSceneGraph::instance()->loadDefaultGeostate();
         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

         geom->setStateSet(state.get());

         std::map<Index, Index> vertexMap;
         std::vector<std::vector<osg::Vec3> > vertexNormals;
         vertexNormals.resize(numVertices);

         Index num = 0;
         for (Index index = 0; index < numElements; index ++) {

            osg::Vec3 vert[3];

            if (index == numElements - 1)
               num = numCorners - el[index];
            else
               num = el[index + 1] - el[index];

            primitives->push_back(num);

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
      if (!name.empty()) {
         s_coverMutex.lock();
         coVRShader *shader = coVRShaderList::instance()->get(name, &parammap);
         shader->apply(geode);
         s_coverMutex.unlock();
      }
   }

   return geode;
}
