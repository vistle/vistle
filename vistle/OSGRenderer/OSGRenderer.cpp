#include <osgGA/TrackballManipulator>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>

#include <osg/Matrix>

#include <osg/Node>

#include "object.h"

#include "OSGRenderer.h"

MODULE_MAIN(OSGRenderer)

OSGRenderer::OSGRenderer(int rank, int size, int moduleID)
   : Renderer("OSGRenderer", rank, size, moduleID), osgViewer::Viewer() {

   setUpViewInWindow(0, 0, 512, 512);
   setLightingMode(osgViewer::Viewer::HEADLIGHT);

   if (!getCameraManipulator() && getCamera()->getAllowEventFocus())
      setCameraManipulator(new osgGA::TrackballManipulator());

   realize();
   setThreadingModel(SingleThreaded);

   scene = new osg::Group();
   setSceneData(scene);

   char title[64];
   snprintf(title, 64, "vistle renderer %d/%d", rank, size);

   std::vector<osgViewer::GraphicsWindow *> windows;
   getWindows(windows);
   std::vector<osgViewer::GraphicsWindow *>::iterator i;
   for (i = windows.begin(); i != windows.end(); i ++)
      (*i)->setWindowName(title);
}

OSGRenderer::~OSGRenderer() {

}

void OSGRenderer::render() {

   float matrix[16];

   if (rank == 0 && size > 1) {

      const osg::Matrixd view = getCameraManipulator()->getMatrix();

      for (int y = 0; y < 4; y ++)
         for (int x = 0; x < 4; x ++)
            matrix[x + y * 4] = view(x, y);
   }

   if (size > 1)
      MPI_Bcast(matrix, 16, MPI_FLOAT, 0, MPI_COMM_WORLD);

   if (rank != 0) {

      osg::Matrix view;

      for (int y = 0; y < 4; y ++)
         for (int x = 0; x < 4; x ++)
            view(x, y) = matrix[x + y * 4];

      getCameraManipulator()->setByMatrix(osg::Matrix(view));
   }

   MPI_Barrier(MPI_COMM_WORLD);

   advance();
   frame();
}

bool OSGRenderer::compute() {

   return true;
}

bool OSGRenderer::addInputObject(const std::string & portName,
                                 const vistle::Object * object) {
   
   switch (object->getType()) {

      case vistle::Object::TRIANGLES: {

         const vistle::Triangles *triangles =
            static_cast<const vistle::Triangles *>(object);
         const size_t numCorners = triangles->getNumCorners();
         const size_t numVertices = triangles->getNumVertices();

         osg::ref_ptr<osg::Geode> geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         for (unsigned int vertex = 0; vertex < numVertices; vertex ++)
            vertices->push_back(osg::Vec3((*triangles->x)[vertex],
                                          (*triangles->y)[vertex],
                                          (*triangles->z)[vertex]));
         geometry->setVertexArray(vertices.get());

         osg::ref_ptr<osg::DrawElementsUInt> corners =
            new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
         for (unsigned int corner = 0; corner < numCorners; corner ++)
            corners->push_back((*triangles->cl)[corner]);
         geometry->addPrimitiveSet(corners.get());

         geode->addDrawable(geometry.get());
         scene->addChild(geode);

         nodes[object->getName()] = geode;
         break;
      }

      case vistle::Object::LINES: {

         const vistle::Lines *lines =
            static_cast<const vistle::Lines *>(object);
         const size_t numElements = lines->getNumElements();
         const size_t numCorners = lines->getNumCorners();

         osg::ref_ptr<osg::Geode> geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
         osg::ref_ptr<osg::DrawArrayLengths> primitives = new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

         int num = 0;
         for (size_t index = 0; index < numElements; index ++) {

            if (index == numElements - 1)
               num = numCorners - (*lines->el)[index];
            else
               num = (*lines->el)[index + 1] - (*lines->el)[index];

            primitives->push_back(num);

            for (int n = 0; n < num; n ++) {
               int v = (*lines->cl)[(*lines->el)[index] + n];
               vertices->push_back(osg::Vec3((*lines->x)[v],
                                             (*lines->y)[v],
                                             (*lines->z)[v]));
            }
         }

         geometry->setVertexArray(vertices.get());
         geometry->addPrimitiveSet(primitives.get());

         geode->addDrawable(geometry.get());
         scene->addChild(geode);

         nodes[object->getName()] = geode;
         break;
      }

      case vistle::Object::POLYGONS: {

         const vistle::Polygons *polygons =
            static_cast<const vistle::Polygons *>(object);
         const size_t numElements = polygons->getNumElements();
         const size_t numCorners = polygons->getNumCorners();

         osg::ref_ptr<osg::Geode> geode = new osg::Geode();
         osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
         osg::ref_ptr<osg::DrawArrayLengths> primitives = new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
         osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();

         int num = 0;
         for (size_t index = 0; index < numElements; index ++) {

            osg::Vec3 vert[3];

            if (index == numElements - 1)
               num = numCorners - (*polygons->el)[index];
            else
               num = (*polygons->el)[index + 1] - (*polygons->el)[index];

            primitives->push_back(num);

            for (int n = 0; n < num; n ++) {
               int v = (*polygons->cl)[(*polygons->el)[index] + n];

               osg::Vec3 vi((*polygons->x)[v], (*polygons->y)[v],
                            (*polygons->z)[v]);
               vertices->push_back(vi);
               if (n < 3)
                  vert[n] = vi;
            }

            osg::Vec3 normal = (vert[2] - vert[0]) ^ (vert[1] - vert[0]);
            normal.normalize();
            for (int n = 0; n < num; n ++)
               normals->push_back(normal);
         }

         geometry->setVertexArray(vertices.get());
         geometry->addPrimitiveSet(primitives.get());
         geometry->setNormalArray(normals.get());
         geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

         geode->addDrawable(geometry.get());
         scene->addChild(geode);

         nodes[object->getName()] = geode;

         break;
      }

      default:
         break;
   }

   return true;
}
