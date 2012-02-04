#include <IceT.h>
#include <IceTMPI.h>

#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventAdapter>
#include <osgGA/EventQueue>

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
#include <osg/Texture1D>
#include <osg/Texture2D>

#include <osgDB/ReadFile>

#include "object.h"

#include "OSGRenderer.h"

MODULE_MAIN(OSGRenderer)

class SyncOperation : public osg::Operation {

public:
   SyncOperation(const int r)
      : osg::Operation("SyncOperation", true), rank(r) { }

   void operator () (osg::Object *object) {

      if (!dynamic_cast<osg::GraphicsContext*>(object))
         return;

      IceTDouble proj[16];
      IceTDouble mv[16];
      IceTFloat bg[4] = { 51.0 / 255.0, 51.0 / 255.0, 102.0 / 255.0, 0.0 };

      IceTImage image = icetDrawFrame(proj, mv, bg);
      IceTSizeType width = icetImageGetWidth(image);
      IceTSizeType height = icetImageGetHeight(image);

      if (rank == 0 && !icetImageIsNull(image)) {

         IceTUByte *color = icetImageGetColorub(image);
         glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, color);
      }

      MPI_Barrier(MPI_COMM_WORLD);
   }

   const int rank;
};

class GUIEvent {

public:
   GUIEvent() {}
   GUIEvent(osgGA::GUIEventAdapter::EventType t,
            osgGA::GUIEventAdapter::ScrollingMotion m,
            double ti):
      type(t), motion(m), time(ti) {
   }

   osgGA::GUIEventAdapter::EventType type;
   osgGA::GUIEventAdapter::ScrollingMotion motion;
   double time;
};

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


TimestepHandler::TimestepHandler()
  : timestep(0), lastEvent(0) {

}

void TimestepHandler::addObject(osg::Geode * geode, const int step) {

   std::vector<osg::Geode *> *vector = NULL;
   std::map<int, std::vector<osg::Geode *> *>::iterator i =
      timesteps.find(step);
   if (i != timesteps.end())
      vector = i->second;
   else {
      vector = new std::vector<osg::Geode *>;
      timesteps[step] = vector;
   }

   if (step != -1 && step != timestep)
      geode->setNodeMask(0);

   vector->push_back(geode);
}

bool TimestepHandler::setTimestepState(const int timestep, const int state) {

   std::map<int, std::vector<osg::Geode *> *>:: iterator ts =
      timesteps.find(timestep);

   if (ts != timesteps.end()) {
      std::vector<osg::Geode *>::iterator i;
      for (i = ts->second->begin(); i != ts->second->end(); i ++)
         (*i)->setNodeMask(state);
      return true;
   } else
      return false;
}

int TimestepHandler::firstTimestep() {

   int index = timesteps.begin()->first;
   if (index == -1)
      return 0;
   return index;
}

int TimestepHandler::lastTimestep() {

   int index = (--timesteps.end())->first;
   if (index == -1)
      return 0;
   return index;
}

bool TimestepHandler::handle(const osgGA::GUIEventAdapter & ea,
                             osgGA::GUIActionAdapter & aa) {

   if (ea.getTime() == lastEvent)
      return true;

   bool handled = false;

   if (ea.getEventType() == osgGA::GUIEventAdapter::SCROLL) {

      lastEvent = ea.getTime();

      switch (ea.getScrollingMotion()) {

      case osgGA::GUIEventAdapter::SCROLL_UP:

         setTimestepState(timestep, 0);
         timestep ++;
         if (timestep > lastTimestep())
            timestep = firstTimestep();
         setTimestepState(timestep, -1);
         handled = true;
         break;

      case osgGA::GUIEventAdapter::SCROLL_DOWN:

         setTimestepState(timestep, 0);
         timestep --;
         if (timestep < firstTimestep() || timestep < 0)
            timestep = lastTimestep();

         setTimestepState(timestep, -1);
         handled = true;
         break;

      default:
         break;
      }
   }
   if (handled)
      aa.requestRedraw();
   return false;
}

void cb(const IceTDouble * proj, const IceTDouble * mv,
        const IceTFloat * bg, const IceTInt * viewport,
        IceTImage result) {

   IceTSizeType width = icetImageGetWidth(result);
   IceTSizeType height = icetImageGetHeight(result);

   //IceTEnum cf = icetImageGetColorFormat(result);
   //IceTEnum df = icetImageGetDepthFormat(result);

   IceTUByte *color = icetImageGetColorub(result);
   IceTFloat *depth = icetImageGetDepthf(result);

   glReadBuffer(GL_BACK);
   glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color);
   glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
}

OSGRenderer::OSGRenderer(int rank, int size, int moduleID)
   : Renderer("OSGRenderer", rank, size, moduleID), osgViewer::Viewer() {

#ifndef _WIN32
   cpu_set_t cpuset;
   CPU_SET(0, &cpuset);
   pthread_setaffinity_np(pthread_self(), 1, &cpuset);
#endif

   IceTCommunicator icetComm;
   IceTContext icetContext;

   icetComm = icetCreateMPICommunicator(MPI_COMM_WORLD);
   icetContext = icetCreateContext(icetComm);
   icetDestroyMPICommunicator(icetComm);

   icetResetTiles();
   icetAddTile(0, 0, 512, 512, 0);
   icetStrategy(ICET_STRATEGY_REDUCE);
   icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
   icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
   icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);

   icetDrawCallback(cb);

   setUpViewInWindow(0, 0, 512, 512);
   setLightingMode(osgViewer::Viewer::HEADLIGHT);

   if (!getCameraManipulator() && getCamera()->getAllowEventFocus())
      setCameraManipulator(new osgGA::TrackballManipulator());

   realize();
   setThreadingModel(SingleThreaded);
   getCamera()->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

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

   addEventHandler(&timesteps);

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

   Contexts ctx;
   getContexts(ctx);
   for (Contexts::iterator c = ctx.begin(); c != ctx.end(); c ++)
      (*c)->add(new SyncOperation(rank));
}

OSGRenderer::~OSGRenderer() {

}

void OSGRenderer::distributeAndHandleEvents() {

   std::vector<GUIEvent> events;

   Contexts ctx;
   getContexts(ctx);
   for (Contexts::iterator c = ctx.begin(); c != ctx.end(); c ++) {

      osgViewer::GraphicsWindow * gw =
         dynamic_cast<osgViewer::GraphicsWindow*>(*c);
      if (gw) {
         gw->checkEvents();
         osgGA::EventQueue::Events ev;
         gw->getEventQueue()->takeEvents(ev);
         osgGA::EventQueue::Events::iterator itr;
         for (itr = ev.begin(); itr != ev.end(); ++itr) {
            osgGA::GUIEventAdapter *event = itr->get();
            if (_cameraManipulator.valid())
               _cameraManipulator->handleWithCheckAgainstIgnoreHandledEventsMask(*event, *this);

            if (!rank &&
                event->getEventType() == osgGA::GUIEventAdapter::SCROLL)
               events.push_back(GUIEvent(osgGA::GUIEventAdapter::SCROLL,
                                         event->getScrollingMotion(),
                                         event->getTime()));
            else {
               EventHandlers::iterator hitr;
               for (hitr = _eventHandlers.begin();
                    hitr != _eventHandlers.end(); hitr ++)
                  (*hitr)->handleWithCheckAgainstIgnoreHandledEventsMask(*event,
                                                                         *this,
                                                                         0, 0);
            }
         }
      }
   }

   unsigned int numEvents = events.size();
   MPI_Bcast(&numEvents, 1, MPI_INT, 0, MPI_COMM_WORLD);
   if (numEvents) {
      if (rank)
         events.resize(numEvents);
      MPI_Bcast(&(events[0]), numEvents * sizeof(GUIEvent), MPI_CHAR, 0,
                MPI_COMM_WORLD);
   }

   std::vector<osgGA::GUIEventAdapter *> osgEvents;
   std::vector<GUIEvent>::iterator i;
   for (i = events.begin(); i != events.end(); i++) {

      if (i->type == osgGA::GUIEventAdapter::SCROLL) {
         osgGA::GUIEventAdapter *event = new osgGA::GUIEventAdapter();
         event->setEventType(osgGA::GUIEventAdapter::SCROLL);
         event->setScrollingMotion(i->motion);
         event->setTime(i->time);
         osgEvents.push_back(event);
      }
   }

   std::vector<osgGA::GUIEventAdapter *>::iterator itr;
   for (itr = osgEvents.begin(); itr != osgEvents.end(); itr++) {

      EventHandlers::iterator hitr;
      for (hitr = _eventHandlers.begin(); hitr != _eventHandlers.end(); hitr ++)
         (*hitr)->handleWithCheckAgainstIgnoreHandledEventsMask(*(*itr),
                                                                *this, 0, 0);
   }
}

void OSGRenderer::distributeModelviewMatrix() {

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
}

void OSGRenderer::render() {

   advance();

   distributeAndHandleEvents();
   distributeModelviewMatrix();

   updateTraversal();
   renderingTraversals();
}

bool OSGRenderer::compute() {

   return true;
}

void OSGRenderer::addInputObject(const vistle::Object * geometry,
                                 const vistle::Object * colors,
                                 const vistle::Object * normals,
                                 const vistle::Object * texture) {

   osg::Geode *geode = NULL;

   if (geometry) {
      switch (geometry->getType()) {

         case vistle::Object::TRIANGLES: {

            const vistle::Triangles *triangles =
               static_cast<const vistle::Triangles *>(geometry);
            const size_t numCorners = triangles->getNumCorners();
            const size_t numVertices = triangles->getNumVertices();

            size_t *cl = &((*triangles->cl)[0]);
            float *x = &((*triangles->x)[0]);
            float *y = &((*triangles->y)[0]);
            float *z = &((*triangles->z)[0]);

            geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            for (unsigned int v = 0; v < numVertices; v ++)
               vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

            geom->setVertexArray(vertices.get());

            osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

            osg::ref_ptr<osg::DrawElementsUInt> corners =
               new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
            for (unsigned int corner = 0; corner < numCorners; corner ++)
               corners->push_back(cl[corner]);

            std::vector<osg::Vec3> * vertexNormals =
               new std::vector<osg::Vec3>[numVertices];

            for (size_t c = 0; c < numCorners; c += 3) {
               osg::Vec3 u(x[cl[c + 0]], y[cl[c + 0]], z[cl[c + 0]]);
               osg::Vec3 v(x[cl[c + 1]], y[cl[c + 1]], z[cl[c + 1]]);
               osg::Vec3 w(x[cl[c + 2]], y[cl[c + 2]], z[cl[c + 2]]);
               osg::Vec3 normal = (w - u) ^ (v - u) * -1;
               normal.normalize();
               vertexNormals[cl[c]].push_back(normal);
               vertexNormals[cl[c + 1]].push_back(normal);
               vertexNormals[cl[c + 2]].push_back(normal);
            }

            for (size_t vertex = 0; vertex < numVertices; vertex ++) {
               osg::Vec3 n;
               std::vector<osg::Vec3>::iterator i;
               for (i = vertexNormals[vertex].begin(); i != vertexNormals[vertex].end(); i ++)
                  n += *i;
               norm->push_back(n);
            }
            delete[] vertexNormals;

            osg::ref_ptr<osg::StateSet> state = new osg::StateSet();
            state->setAttributeAndModes(material.get(),
                                        osg::StateAttribute::ON);
            state->setAttributeAndModes(lightModel.get(),
                                        osg::StateAttribute::ON);
            state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

            if (texture && texture->getType() == vistle::Object::TEXTURE1D) {

               const vistle::Texture1D *tex =
                  static_cast<const vistle::Texture1D *>(texture);

               osg::ref_ptr<osg::Texture1D> osgTex = new osg::Texture1D;
               osgTex->setDataVariance(osg::Object::DYNAMIC);

               osg::ref_ptr<osg::Image> image = new osg::Image();

               image->setImage(tex->getWidth(), 1, 1, GL_RGBA, GL_RGBA,
                               GL_UNSIGNED_BYTE, &(*tex->pixels)[0],
                               osg::Image::NO_DELETE);
               osgTex->setImage(image);

               osg::ref_ptr<osg::FloatArray> coords = new osg::FloatArray();
               std::copy(tex->coords->begin(), tex->coords->end(),
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

            scene->addChild(geode);

            nodes[geometry->getName()] = geode;
            break;
         }

         case vistle::Object::LINES: {

            const vistle::Lines *lines =
               static_cast<const vistle::Lines *>(geometry);
            const size_t numElements = lines->getNumElements();
            const size_t numCorners = lines->getNumCorners();

            size_t *el = &((*lines->el)[0]);
            size_t *cl = &((*lines->cl)[0]);
            float *x = &((*lines->x)[0]);
            float *y = &((*lines->y)[0]);
            float *z = &((*lines->z)[0]);

            geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
            osg::ref_ptr<osg::DrawArrayLengths> primitives =
               new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

            int num = 0;
            for (size_t index = 0; index < numElements; index ++) {

               if (index == numElements - 1)
                  num = numCorners - el[index];
               else
                  num = el[index + 1] - el[index];

               primitives->push_back(num);

               for (int n = 0; n < num; n ++) {
                  int v = cl[el[index] + n];
                  vertices->push_back(osg::Vec3(x[v], y[v], z[v]));
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

            size_t *el = &((*polygons->el)[0]);
            size_t *cl = &((*polygons->cl)[0]);
            float *x = &((*polygons->x)[0]);
            float *y = &((*polygons->y)[0]);
            float *z = &((*polygons->z)[0]);
            float *nx = NULL;
            float *ny = NULL;
            float *nz = NULL;
            if (numNormals) {
               nx = &((*vec->x)[0]);
               ny = &((*vec->y)[0]);
               nz = &((*vec->z)[0]);
            }

            geode = new osg::Geode();
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
            osg::ref_ptr<osg::DrawArrayLengths> primitives =
               new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            osg::ref_ptr<osg::Vec3Array> norm = new osg::Vec3Array();

            osg::ref_ptr<osg::StateSet> state = new osg::StateSet();
            state->setAttributeAndModes(material.get(),
                                        osg::StateAttribute::ON);
            state->setAttributeAndModes(lightModel.get(),
                                        osg::StateAttribute::ON);
            state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

            geom->setStateSet(state.get());

            std::map<int, int> vertexMap;
            std::vector<osg::Vec3> vertexNormals[numVertices];

            int num = 0;
            for (size_t index = 0; index < numElements; index ++) {

               osg::Vec3 vert[3];

               if (index == numElements - 1)
                  num = numCorners - el[index];
               else
                  num = el[index + 1] - el[index];

               primitives->push_back(num);

               for (int n = 0; n < num; n ++) {
                  int v = cl[el[index] + n];

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
                  for (int n = 0; n < num; n ++)
                     vertexNormals[cl[el[index] + n]].push_back(no);
               }
            }

            // convert per face normals to per vertex normals
            for (size_t vertex = 0; vertex < vertices->size(); vertex ++) {
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

      if (geode)
         timesteps.addObject(geode, geometry->getTimestep());
   }
}


bool OSGRenderer::addInputObject(const std::string & portName,
                                 const vistle::Object * object) {

   std::cout << "++++++OSGRenderer addInputObject " << object->getType()
             << " block " << object->getBlock()
             << " timestep " << object->getTimestep() << std::endl;

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
