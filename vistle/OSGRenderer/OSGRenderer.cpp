#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventHandler>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/DisplaySettings>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
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

   material = new osg::Material();
   material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
   material->setAmbient(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(0.2f, 0.2f, 0.2f, 1.0));
   material->setDiffuse(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(1.0f, 1.0f, 1.0f, 1.0));
   material->setSpecular(osg::Material::FRONT_AND_BACK,
                         osg::Vec4(0.4f, 0.4f, 0.4f, 1.0));
   material->setEmission(osg::Material::FRONT_AND_BACK,
                         osg::Vec4(0.0f, 0.0f, 0.0f, 1.0));
   material->setShininess(osg::Material::FRONT_AND_BACK, 16.0f);

   lightModel = new osg::LightModel;
   lightModel->setTwoSided(true);
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

void OSGRenderer::addInputObject(const vistle::Object * geometry,
                                 const vistle::Object * colors,
                                 const vistle::Object * normals,
                                 const vistle::Object * texture) {

   if (geometry) {
      switch (geometry->getType()) {

         case vistle::Object::TRIANGLES: {

            const vistle::Triangles *triangles =
               static_cast<const vistle::Triangles *>(geometry);
            const size_t numCorners = triangles->getNumCorners();
            const size_t numVertices = triangles->getNumVertices();

            osg::ref_ptr<osg::Geode> geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            for (unsigned int vertex = 0; vertex < numVertices; vertex ++)
               vertices->push_back(osg::Vec3((*triangles->x)[vertex],
                                             (*triangles->y)[vertex],
                                             (*triangles->z)[vertex]));
            geom->setVertexArray(vertices.get());

            osg::ref_ptr<osg::DrawElementsUInt> corners =
               new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
            for (unsigned int corner = 0; corner < numCorners; corner ++)
               corners->push_back((*triangles->cl)[corner]);
            geom->addPrimitiveSet(corners.get());

            geode->addDrawable(geom.get());
            scene->addChild(geode);

            nodes[geometry->getName()] = geode;
            break;
         }

         case vistle::Object::LINES: {

            const vistle::Lines *lines =
               static_cast<const vistle::Lines *>(geometry);
            const size_t numElements = lines->getNumElements();
            const size_t numCorners = lines->getNumCorners();

            osg::ref_ptr<osg::Geode> geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
            osg::ref_ptr<osg::DrawArrayLengths> primitives =
               new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

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

            geom->setVertexArray(vertices.get());
            geom->addPrimitiveSet(primitives.get());

            geode->addDrawable(geom.get());
            scene->addChild(geode);

            nodes[geometry->getName()] = geode;
            break;
         }

         case vistle::Object::POLYGONS: {

            const vistle::Polygons *polygons =
               static_cast<const vistle::Polygons *>(geometry);

            const vistle::Vec3<float> *vec = NULL;
            if (normals && normals->getType() == vistle::Object::VEC3FLOAT)
               vec = static_cast<const vistle::Vec3<float> *>(normals);

            const size_t numElements = polygons->getNumElements();
            const size_t numCorners = polygons->getNumCorners();
            const size_t numVertices = polygons->getNumVertices();
            const size_t numNormals = vec ? vec->getSize() : 0;

            osg::ref_ptr<osg::Geode> geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
            osg::ref_ptr<osg::DrawArrayLengths> primitives = new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

            osg::ref_ptr<osg::StateSet> state = new osg::StateSet();
            state->setAttributeAndModes(material.get(),
                                        osg::StateAttribute::ON);
            state->setAttributeAndModes(lightModel.get(),
                                        osg::StateAttribute::ON);
            state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

            geom->setStateSet(state.get());

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

                  if (numNormals) {
                     osg::Vec3 no((*vec->x)[v], (*vec->y)[v], (*vec->z)[v]);
                     no.normalize();
                     norm->push_back(no);
                  }
                  else if (n < 3)
                     vert[n] = vi;
               }

               if (!numNormals) {
                  osg::Vec3 normal =
                     (vert[2] - vert[0]) ^ (vert[1] - vert[0]) * -1;
                  normal.normalize();
                  for (int n = 0; n < num; n ++)
                     norm->push_back(normal);
               }
            }

            geom->setVertexArray(vertices.get());
            geom->addPrimitiveSet(primitives.get());
            geom->setNormalArray(norm.get());
            geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

            geode->addDrawable(geom.get());
            scene->addChild(geode);

            nodes[geometry->getName()] = geode;
            break;
         }

         case vistle::Object::SET: {

            const vistle::Set *gset =
               static_cast<const vistle::Set *>(geometry);
            const vistle::Set *cset = static_cast<const vistle::Set *>(colors);
            const vistle::Set *nset = static_cast<const vistle::Set *>(normals);
            const vistle::Set *tset = static_cast<const vistle::Set *>(texture);

            for (size_t index = 0; index < gset->elements->size(); index ++) {
               addInputObject(gset->getElement(index),
                              cset ? cset->getElement(index) : NULL,
                              nset ? nset->getElement(index) : NULL,
                              tset ? tset->getElement(index) : NULL);
            }
            break;
         }

         default:
            break;
      }
   }
}


bool OSGRenderer::addInputObject(const std::string & portName,
                                 const vistle::Object * object) {

   std::cout << "OSGRenderer addInputObject " << object->getType()
             << std::endl;

   switch (object->getType()) {

      case vistle::Object::TRIANGLES:
      case vistle::Object::POLYGONS:
      case vistle::Object::LINES: {

         addInputObject(object, NULL, NULL, NULL);
         break;
      }

      case vistle::Object::SET: {

         const vistle::Set *set = static_cast<const vistle::Set *>(object);
         for (size_t index = 0; index < set->elements->size(); index ++) {
            addInputObject(portName, set->getElement(index));
         }
         break;
      }

      case vistle::Object::GEOMETRY: {

         const vistle::Geometry *geom =
            static_cast<const vistle::Geometry *>(object);

         addInputObject(&*geom->geometry, &*geom->colors, &*geom->normals,
                        &*geom->texture);
         break;
      }

      default:
         break;
   }

   return true;
}
