#include "VistleGeometryGenerator.h"
#include <kernel/VRSceneGraph.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture1D>

#include <core/polygons.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/texture1d.h>
#include <core/placeholder.h>

using namespace opencover;
using namespace vistle;

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

osg::Node *VistleGeometryGenerator::operator()() {

   if (!m_geo)
      return NULL;

   osg::Geode *geode = NULL;

   switch (m_geo->getType()) {

      case vistle::Object::PLACEHOLDER: {
         vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
         switch (ph->originalType()) {
            case vistle::Object::TRIANGLES:
            case vistle::Object::LINES:
            case vistle::Object::POLYGONS: {
               osg::Node *node = new osg::Node();
               node->setName(ph->originalName());
               return node;
               break;
            }
         default:
            break;
         }
         break;
      }

      case vistle::Object::TRIANGLES: {

         vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

         const Index numCorners = triangles->getNumCorners();
         const Index numVertices = triangles->getNumVertices();

         std::cerr << "   Triangles: [ #c " << numCorners << ", #v " << numVertices << " ]" << std::endl;

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

         osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

         osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
         for (Index corner = 0; corner < numCorners; corner ++)
            corners->push_back(cl[corner]);

         std::vector<osg::Vec3> * vertexNormals = new std::vector<osg::Vec3>[numVertices];

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

         std::cerr << "   Lines: [ #c " << numCorners << ", #e " << numElements << " ]" << std::endl;

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

         Index num = 0;
         for (Index index = 0; index < numElements; index ++) {

            if (index == numElements - 1)
               num = numCorners - el[index];
            else
               num = el[index + 1] - el[index];

            primitives->push_back(num);

            for (Index n = 0; n < num; n ++) {
               Index v = cl[el[index] + n];
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

         vistle::Vec<vistle::Scalar,3>::const_ptr vec;
         if (m_normal)
            vec = vistle::Vec<vistle::Scalar,3>::as(m_normal);

         const Index numElements = polygons->getNumElements();
         const Index numCorners = polygons->getNumCorners();
         const Index numVertices = polygons->getNumVertices();
         const Index numNormals = vec ? vec->getSize() : 0;

         std::cerr << "   Polygons: [ #c " << numCorners << ", #e " << numElements << ", #v " << numVertices << " ]" << std::endl;

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
         break;
   }

   if (geode)
      geode->setName(m_geo->getName());

   return geode;
}
