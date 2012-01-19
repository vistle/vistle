#include <osgGA/TrackballManipulator>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>

#include <osg/Matrix>

#include <osg/Node>

#include "osgrenderer.h"

MODULE_MAIN(OSGRenderer)

OSGRenderer::OSGRenderer(int rank, int size, int moduleID)
   : Renderer("OSGRenderer", rank, size, moduleID), osgViewer::Viewer() {

   printf("renderer\n");
   createInputPort("data_in");
   setUpViewInWindow(0, 0, 512, 512);
   setLightingMode(osgViewer::Viewer::HEADLIGHT);

   //if (rank == 0) {
      if (!getCameraManipulator() && getCamera()->getAllowEventFocus())
         setCameraManipulator(new osgGA::TrackballManipulator());
      /*
   } else {
      getCamera()->setAllowEventFocus(false);
   }
      */

   realize();
   setThreadingModel(SingleThreaded);

   osg::ref_ptr<osg::Group> root = new osg::Group();
   osg::ref_ptr<osg::Geode> geode = new osg::Geode();
   osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

   geode->addDrawable(geometry.get());
   root->addChild(geode);

   osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
   vertices->push_back(osg::Vec3(0.0, 0.0, 0.0));
   vertices->push_back(osg::Vec3(1.0, 0.0, 0.0));
   vertices->push_back(osg::Vec3(1.0, 1.0, 0.0));

   geometry->setVertexArray(vertices.get());

   osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
   corners->push_back(0);
   corners->push_back(1);
   corners->push_back(2);

   geometry->addPrimitiveSet(corners.get());

   setSceneData(root.get());

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

   float matrix[32];

   if (rank == 0) {

      const osg::Matrixd view = getCamera()->getViewMatrix();
      const osg::Matrixd proj = getCamera()->getProjectionMatrix();

      for (int y = 0; y < 4; y ++)
         for (int x = 0; x < 4; x ++) {
            matrix[x + y * 4] = view(x, y);
            matrix[x + y * 4 + 16] = proj(x, y);
         }
   }

   MPI_Bcast(matrix, 32, MPI_FLOAT, 0, MPI_COMM_WORLD);

   if (rank != 0) {

      //getCamera()->setViewMatrix(osg::Matrix(matrix));
      getCameraManipulator()->setByMatrix(osg::Matrix(matrix));
      getCamera()->setProjectionMatrix(osg::Matrix(matrix + 16));

      for (int y = 0; y < 4; y ++) {
         for (int x = 0; x < 4; x ++)
            printf("%0.4f ", matrix[x + y * 4]);
         printf("\n");
      }
      printf("\n");
   }

   MPI_Barrier(MPI_COMM_WORLD);

   advance();
   frame();
}
