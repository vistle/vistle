#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventHandler>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/DisplaySettings>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Image>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/Projection>
#include <osg/GraphicsContext>
#include <osg/Texture2D>

#include <osgDB/ReadFile>

#include "object.h"

#include "OSGRenderer.h"

MODULE_MAIN(OSGRenderer)

class ResizeHandler: public osgGA::GUIEventHandler {

public:
   ResizeHandler(osg::ref_ptr<osg::Projection> p,
                 osg::ref_ptr<osg::MatrixTransform> m)
      : osgGA::GUIEventHandler(),
        projection(p), modelView(m) {
   }

   bool handle(const osgGA::GUIEventAdapter & ea,
               osgGA::GUIActionAdapter & aa) {

      switch (ea.getEventType()) {

         case osgGA::GUIEventAdapter::RESIZE: {

            int width = ea.getWindowWidth();
            int height = ea.getWindowHeight();
            projection->setMatrix(osg::Matrix::ortho2D(0, width, 0, height));
            /*
            projection->setMatrix(osg::Matrix::ortho2D(0, width, 0, height));

            vertices->clear();
            vertices->push_back(osg::Vec3(width - 128 - 4, height - 64 - 4,
                                          -1.0));
            vertices->push_back(osg::Vec3(width - 4, height - 64 - 4, -1.0));
            vertices->push_back(osg::Vec3(width - 4, height - 4, -1.0));
            vertices->push_back(osg::Vec3(width - 128 - 4, height - 4, -1.0));
            */
            break;
         }

         default:
            break;
      }
      return false;
   }

private:
   osg::ref_ptr<osg::Projection> projection;
   osg::ref_ptr<osg::MatrixTransform> modelView;
};

OSGRenderer::OSGRenderer(int rank, int size, int moduleID)
   : Renderer("OSGRenderer", rank, size, moduleID), osgViewer::Viewer() {

   setUpViewInWindow(0, 0, 512, 512);
   setLightingMode(osgViewer::Viewer::HEADLIGHT);

   if (!getCameraManipulator() && getCamera()->getAllowEventFocus())
      setCameraManipulator(new osgGA::TrackballManipulator());

   realize();
   setThreadingModel(SingleThreaded);

   scene = new osg::Group();

   osg::ref_ptr<osg::Geode> geode = new osg::Geode();

   osg::ref_ptr<osg::Image> vistleImage =
      osgDB::readImageFile("../vistle.png");

   osg::ref_ptr<osg::Texture2D> texture =
      new osg::Texture2D(vistleImage.get());
   texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
   texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

   osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();

   stateSet->setTextureAttributeAndModes(0, texture.get(),
                                         osg::StateAttribute::ON);

   stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

   osg::ref_ptr<osg::BlendFunc> blend = new osg::BlendFunc();
   stateSet->setAttributeAndModes(blend.get(), osg::StateAttribute::ON);

   osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc();
   alpha->setFunction(osg::AlphaFunc::GEQUAL, 0.05);
   stateSet->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON);
   stateSet->setRenderBinDetails(11, "RenderBin");
   stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

   osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
   osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
   vertices->push_back(osg::Vec3(4.0, 4.0, -1.0));
   vertices->push_back(osg::Vec3(132.0, 4.0, -1.0));
   vertices->push_back(osg::Vec3(132.0, 68.0, -1.0));
   vertices->push_back(osg::Vec3(4.0, 68.0, -1.0));

   osg::ref_ptr<osg::DrawElementsUInt> indices
      = new osg::DrawElementsUInt(osg::PrimitiveSet::POLYGON, 0);
   indices->push_back(0);
   indices->push_back(1);
   indices->push_back(2);
   indices->push_back(3);

   osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
   texCoords->push_back(osg::Vec2(0.0, 0.0));
   texCoords->push_back(osg::Vec2(1.0, 0.0));
   texCoords->push_back(osg::Vec2(1.0, 1.0));
   texCoords->push_back(osg::Vec2(0.0, 1.0));

   geometry->setVertexArray(vertices.get());
   geometry->addPrimitiveSet(indices.get());
   geometry->setTexCoordArray(0, texCoords.get());
   geometry->setStateSet(stateSet.get());

   geode->addDrawable(geometry.get());

   osg::ref_ptr<osg::Projection> proj = new osg::Projection();
   proj->setMatrix(osg::Matrix::ortho2D(0, 512, 0, 512));

   osg::ref_ptr<osg::MatrixTransform> view = new osg::MatrixTransform();
   view->setMatrix(osg::Matrix::identity());
   view->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

   view->addChild(geode.get());
   proj->addChild(view.get());
   addEventHandler(new ResizeHandler(proj, view));

   scene->addChild(proj.get());

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

      case vistle::Object::SET: {

         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         for (size_t index = 0; index < set->elements->size(); index ++) {
            addInputObject(portName, set->getElement(index));
         }
         break;
      }

      default:
         printf("Unsupported object type [%d]\n", object->getType());
         break;
   }

   return true;
}
