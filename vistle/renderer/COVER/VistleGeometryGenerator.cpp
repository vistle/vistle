#include "VistleGeometryGenerator.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture1D>
#include <osg/Point>
#include <osg/LineWidth>
#include <osg/MatrixTransform>

#include <core/polygons.h>
#include <core/points.h>
#include <core/spheres.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/texture1d.h>
#include <core/placeholder.h>
#include <core/normals.h>

#include <algorithm>
#include <type_traits>

#ifdef COVER_PLUGIN
#include <cover/RenderObject.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRShader.h>
#include <cover/coVRPluginSupport.h>
#include <PluginUtil/coSphere.h>
#endif

namespace  {
const int NumPrimitives = 10000;
const bool IndexGeo = true;
}

using namespace vistle;

std::mutex VistleGeometryGenerator::s_coverMutex;

template<class Geo>
struct BinAdapter {
    BinAdapter(typename Geo::const_ptr geo)
    : m_geo(geo)
    , m_numPrim(geo->getNumElements())
    {
    }

    Index getNumPrimitives() const {
        return m_numPrim;
    }

    Vector getCoord(Index prim) const;

    typename Geo::const_ptr m_geo;
    Index m_numPrim = 0;
};

template<>
Vector BinAdapter<Triangles>::getCoord(Index prim) const {

    Index c = 3*prim;
    if (m_geo->getNumCorners() > 0)
        c = m_geo->cl()[c];

    return Vector(m_geo->x()[c], m_geo->y()[c], m_geo->z()[c]);
}

struct PrimitiveBin {
    std::vector<Index> prim; // primitive indices
    std::vector<Index> vertexMap; // map from old to new vertex indices
    std::vector<Index> ncl;  // new connectivity list

    void clear() {
        std::vector<Index>().swap(vertexMap);
        std::vector<Index>().swap(ncl);
    }
};

template<class Geo>
std::vector<PrimitiveBin> binPrimitivesRec(int level, const BinAdapter<Geo> &adp, const Vector &bmin, const Vector &bmax, const PrimitiveBin &bin)
{
    if (bin.prim.size() < NumPrimitives || level > 8)
    {
        std::vector<PrimitiveBin> result;
        if (!bin.prim.empty())
            result.push_back(bin);
        return result;
    }

    auto bsize = bmax - bmin;
    int splitDim = 0;
    if (bsize[1] > bsize[splitDim])
        splitDim = 1;
    if (bsize[2] > bsize[splitDim])
        splitDim = 2;

    auto splitAt = 0.5*bmin[splitDim] + 0.5*bmax[splitDim];

    PrimitiveBin binMin, binMax;
    binMin.prim.reserve(bin.prim.size());
    binMax.prim.reserve(bin.prim.size());
    auto splitMin = bmax[splitDim], splitMax = bmin[splitDim];
    for (auto v: bin.prim) {
        auto c = adp.getCoord(v)[splitDim];
        if (c < splitAt) {
            binMin.prim.push_back(v);
            if (c > splitMax)
                splitMax = c;
        } else {
            binMax.prim.push_back(v);
            if (c < splitMin)
                splitMin = c;
        }
    }

    //std::cerr << "level=" << level << ", splitDim=" << splitDim << splitAt << ", #bmin=" << binMin.prim.size() << ", #bmax=" << binMax.prim.size() << std::endl;

    auto smax = bmax;
    smax[splitDim] = splitMax;
    auto rMin = binPrimitivesRec(level+1, adp, bmin, smax, binMin);

    auto smin = bmin;
    smin[splitDim] = splitMin;
    auto rMax = binPrimitivesRec(level+1, adp, smin, bmax, binMax);

    std::copy(rMin.begin(), rMin.end(), std::back_inserter(rMax));

    return rMax;
}

template<class Geo>
std::vector<PrimitiveBin> binPrimitives(typename Geo::const_ptr geo) {
    BinAdapter<Geo> adp(geo);
    auto bounds = geo->getBounds();

    PrimitiveBin bin;
#if 0
    if (adp.getNumPrimitives() < NumPrimitives)
    {
        std::vector<PrimitiveBin> result{bin};
        return result;
    }
#endif
    bin.prim.reserve(adp.getNumPrimitives());
    for (Index i=0; i<adp.getNumPrimitives(); ++i)
        bin.prim.emplace_back(i);
    return binPrimitivesRec(0, adp, bounds.first, bounds.second, bin);
}

VistleGeometryGenerator::VistleGeometryGenerator(std::shared_ptr<vistle::RenderObject> ro,
            vistle::Object::const_ptr geo,
            vistle::Object::const_ptr normal,
            vistle::Object::const_ptr tex)
: m_ro(ro)
, m_geo(geo)
, m_normal(normal)
, m_tex(tex)
{
#ifdef COVER_PLUGIN
    s_coverMutex.lock();
    if (!s_kdtree)
        s_kdtree = new osg::KdTreeBuilder;
    s_coverMutex.unlock();
#endif
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

template<class Array, class Mapped, bool normalize>
struct DataAdapter;

template<class Mapped, bool normalize>
struct DataAdapter<osg::Vec3Array, Mapped, normalize> {
    DataAdapter(vistle::Triangles::const_ptr tri, typename Mapped::const_ptr mapped)
        : x(&mapped->x()[0])
        , y(&mapped->y()[0])
        , z(&mapped->z()[0])
        , mapping(mapped->guessMapping(tri))
    {}
    osg::Vec3 getValue(Index idx) {
        osg::Vec3 val(x[idx], y[idx], z[idx]);
        if (normalize)
            val.normalize();
        return val;
    }
    const typename Mapped::Scalar *x, *y, *z;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Mapped, bool normalize>
struct DataAdapter<osg::FloatArray, Mapped, normalize> {
    DataAdapter(vistle::Triangles::const_ptr tri, typename Mapped::const_ptr mapped)
        : x(&mapped->x()[0])
        , mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) {
        return x[idx];
    }
    const typename Mapped::Scalar *x;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<bool normalize>
struct DataAdapter<osg::Vec3Array, osg::Vec3Array, normalize> {
    DataAdapter(vistle::Triangles::const_ptr tri, osg::Vec3Array *mapped)
    : mapped(mapped)
    {}
    osg::Vec3 getValue(Index idx) {
        osg::Vec3 val = (*mapped)[idx];
        if (normalize)
            val.normalize();
        return  val;
    }
    vistle::DataBase::Mapping mapping = vistle::DataBase::Vertex;
    osg::Vec3Array *mapped = nullptr;

};

template<class MappedPtr, class Array, bool normalize>
Array *applyTriangle(vistle::Triangles::const_ptr tri, MappedPtr mapped, bool indexGeom, PrimitiveBin &bin) {
    if (!mapped)
        return nullptr;

    auto arr = new Array;
    arr->setBinding(osg::Array::BIND_PER_VERTEX);
    DataAdapter<Array, typename std::remove_reference<decltype(*mapped)>::type, normalize> adap(tri, mapped);
    if (adap.mapping == vistle::DataBase::Unspecified)
        return nullptr;

    if (bin.prim.empty()) {
        Index nv = tri->getNumCoords();
        if (adap.mapping == vistle::DataBase::Vertex)
        {
            if (indexGeom || tri->getNumCorners()==0) {
                arr->reserve(nv);
                for (Index v = 0; v < nv; ++v) {
                    arr->push_back(adap.getValue(v));
                }
            } else {
                nv = tri->getNumCorners();
                arr->reserve(nv);
                const auto cl = &tri->cl()[0];
                for (Index i = 0; i < nv; ++i) {
                    Index v = cl[i];
                    arr->push_back(adap.getValue(v));
                }
            }
        } else if (adap.mapping == vistle::DataBase::Element) {
            arr->reserve(nv);
            for (Index face = 0; face < nv/3; ++face) {
                auto val = adap.getValue(face);
                arr->push_back(val);
                arr->push_back(val);
                arr->push_back(val);
            }
        }
    } else {
        const auto cl = &tri->cl()[0];
        bool buildConn = bin.ncl.empty();
        Index vcount = 0;
        for (Index prim: bin.prim) {
            if (adap.mapping == vistle::DataBase::Vertex) {
                Index begin = prim*3, end=begin+3;
                if (indexGeom) {
                    for (Index i = begin; i < end; ++i) {
                        Index v = cl[i];
                        Index vm = bin.vertexMap[v];
                        if (vm == InvalidIndex) {
                            vm = vcount;
                            bin.vertexMap[v] = vm;
                        }
                        if (buildConn)
                            bin.ncl.push_back(vm);
                        if (vm == vcount) {
                            ++vcount;

                            arr->push_back(adap.getValue(v));
                        }
                    }
                } else if (tri->getNumCorners() == 0) {
                    for (Index v = begin; v < end; ++v) {
                        arr->push_back(adap.getValue(v));
                    }
                } else {
                    for (Index i = begin; i < end; ++i) {
                        Index v = cl[i];
                        arr->push_back(adap.getValue(v));
                    }
                }
            } else if (adap.mapping == vistle::DataBase::Element) {
                auto val = adap.getValue(prim);
                arr->push_back(val);
                arr->push_back(val);
                arr->push_back(val);
            }
        }
    }

    return arr;
}

osg::MatrixTransform *VistleGeometryGenerator::operator()(osg::ref_ptr<osg::StateSet> defaultState) {

   if (!m_geo)
      return NULL;

   std::string nodename = m_geo->getName();

   osg::MatrixTransform *transform = new osg::MatrixTransform();
   transform->setName(nodename+".transform");
   osg::Matrix osgMat;
   vistle::Matrix4 vistleMat = m_geo->getTransform();
   for (int i=0; i<4; ++i) {
       for (int j=0; j<4; ++j) {
           osgMat(i,j) = vistleMat.col(i)[j];
       }
   }
   transform->setMatrix(osgMat);

   std::stringstream debug;
   debug << nodename << " ";
   debug << "[";
   debug << (m_geo ? "G" : ".");
   debug << (m_normal ? "N" : ".");
   debug << (m_tex ? "T" : ".");
   debug << "] ";

   int t=m_geo->getTimestep();
   if (t<0 && m_normal)
      t = m_normal->getTimestep();
   if (t<0 && m_tex)
      t = m_tex->getTimestep();

   int b=m_geo->getBlock();
   if (b<0 && m_normal)
      b = m_normal->getBlock();
   if (b<0 && m_tex)
      b = m_tex->getBlock();

   debug << "b " << b << ", t " << t << "  ";

   osg::ref_ptr<osg::Geode> geode = new osg::Geode();
   geode->setName(nodename);
   transform->addChild(geode);

   std::vector<osg::ref_ptr<osg::Drawable>> draw;
   osg::ref_ptr<osg::StateSet> state;
#ifdef COVER_PLUGIN
   opencover::coSphere *sphere = nullptr;
#endif

   bool transparent = false;
   osg::Material *mat = nullptr;
   if (m_ro && m_ro->hasSolidColor) {
       const auto &c = m_ro->solidColor;
       mat = new osg::Material;
       mat->setName(nodename+".material");
       mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(c[0], c[1], c[2], c[3]));
       mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(c[0], c[1], c[2], c[3]));
       mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2, 0.2, 0.2, c[3]));
       mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
       if (c[3] > 0.f && c[3] < 1.f)
           transparent = true;
   }

   if (defaultState) {
      state = new osg::StateSet(*defaultState);
      state->setName(nodename+".state");
   } else {
#ifdef COVER_PLUGIN
      if (transparent) {
          state = opencover::VRSceneGraph::instance()->loadTransparentGeostate();
      } else {
          state = opencover::VRSceneGraph::instance()->loadDefaultGeostate();
      }
#else
      state = new osg::StateSet;
      state->setName(nodename+".state");
      if (transparent) {
          state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
          state->setMode(GL_BLEND, osg::StateAttribute::ON);
      }
#endif
   }

   if (state && mat)
       state->setAttribute(mat);

   bool indexGeom = IndexGeo;
   vistle::Normals::const_ptr normals = vistle::Normals::as(m_normal);
   if (normals) {
       auto m = normals->guessMapping(m_geo);
       if (m != vistle::DataBase::Vertex)
       {
           indexGeom = false;
           debug << " NoIndex: normals";
       }
   }
   vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_tex);
   if (tex) {
       auto m = tex->guessMapping();
       if (m != vistle::DataBase::Vertex)
       {
           indexGeom = false;
           debug << " NoIndex: tex";
       }
   }

   switch (m_geo->getType()) {

      case vistle::Object::PLACEHOLDER: {

         vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
         //debug << "Placeholder [" << ph->originalName() << "]";
         if (isSupported(ph->originalType())) {
            nodename = ph->originalName();
            geode->setName(nodename);
         }
         break;
      }

      case vistle::Object::POINTS: {

         indexGeom = false;

         vistle::Points::const_ptr points = vistle::Points::as(m_geo);
         const Index numVertices = points->getNumPoints();

         debug << "Points: [ #v " << numVertices << " ]";

         auto geom = new osg::Geometry();
         draw.push_back(geom);

         if (numVertices > 0) {
             const vistle::Scalar *x = &points->x()[0];
             const vistle::Scalar *y = &points->y()[0];
             const vistle::Scalar *z = &points->z()[0];
             osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
             for (Index v = 0; v < numVertices; v ++)
                 vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

             geom->setVertexArray(vertices.get());
             geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, numVertices));

             state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
             state->setAttribute(new osg::Point(2.0f), osg::StateAttribute::ON);
         }
         break;
      }

#ifdef COVER_PLUGIN
      case vistle::Object::SPHERES: {
         indexGeom = false;

         vistle::Spheres::const_ptr spheres = vistle::Spheres::as(m_geo);
         const Index numVertices = spheres->getNumSpheres();

         debug << "Spheres: [ #v " << numVertices << " ]";

         sphere = new opencover::coSphere();
         draw.push_back(sphere);

         const vistle::Scalar *x = &spheres->x()[0];
         const vistle::Scalar *y = &spheres->y()[0];
         const vistle::Scalar *z = &spheres->z()[0];
         const vistle::Scalar *r = &spheres->r()[0];
         sphere->setCoords(numVertices, x, y, z, r);
         sphere->setColorBinding(opencover::Bind::OverAll);
         float rgba[] = { 1., 0., 0., 1. };
         sphere->updateColors(&rgba[0], &rgba[1], &rgba[2], &rgba[3]);

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
         sphere->setStateSet(state.get());

         break;
      }
#endif

      case vistle::Object::TRIANGLES: {

         vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

         const Index numCorners = triangles->getNumCorners();
         const Index numVertices = triangles->getNumVertices();
         if (numCorners == 0)
             indexGeom = false;

         debug << "Triangles: [ #c " << numCorners << ", #v " << numVertices << " ]";

         const Index *cl = &triangles->cl()[0];
         const vistle::Scalar *x = &triangles->x()[0];
         const vistle::Scalar *y = &triangles->y()[0];
         const vistle::Scalar *z = &triangles->z()[0];

         osg::Vec3Array *gnormals = nullptr;
         auto normals = vistle::Normals::as(m_normal);
         if (!normals)
         {
             gnormals = new osg::Vec3Array;
             if (numCorners > 0) {
                 gnormals->resize(indexGeom ? numVertices : numCorners);
                 for (Index c = 0; c < numCorners; c += 3) {
                     Index v0 = cl[c+0], v1 = cl[c+1], v2 = cl[c+2];
                     osg::Vec3 u(x[v0], y[v0], z[v0]);
                     osg::Vec3 v(x[v1], y[v1], z[v1]);
                     osg::Vec3 w(x[v2], y[v2], z[v2]);
                     osg::Vec3 normal = (w - u) ^ (v - u) * -1;
                     normal.normalize();
                     if (indexGeom) {
                         (*gnormals)[v0] += normal;
                         (*gnormals)[v1] += normal;
                         (*gnormals)[v2] += normal;
                     } else {
                         (*gnormals)[c+0] = normal;
                         (*gnormals)[c+1] = normal;
                         (*gnormals)[c+2] = normal;
                     }
                 }
                 if (indexGeom) {
                     for (Index v=0; v<numVertices; ++v)
                         (*gnormals)[v].normalize();
                 }
             } else {
                 for (Index c = 0; c < numVertices; c += 3) {
                     osg::Vec3 u(x[c + 0], y[c + 0], z[c + 0]);
                     osg::Vec3 v(x[c + 1], y[c + 1], z[c + 1]);
                     osg::Vec3 w(x[c + 2], y[c + 2], z[c + 2]);
                     osg::Vec3 normal = (w - u) ^ (v - u) * -1;
                     normal.normalize();
                     gnormals->push_back(normal);
                     gnormals->push_back(normal);
                     gnormals->push_back(normal);
                 }
             }
         }

         auto bins = binPrimitives<vistle::Triangles>(triangles);
         debug << " #bins: " << bins.size();

         for (auto bin: bins) {
             auto geom = new osg::Geometry();
             draw.push_back(geom);

             bin.vertexMap.resize(triangles->getNumCorners(), InvalidIndex);
             osg::ref_ptr<osg::Vec3Array> vertices = applyTriangle<Triangles::const_ptr, osg::Vec3Array, false>(triangles, triangles, indexGeom, bin);
             geom->setVertexArray(vertices);

             if (bin.prim.empty()) {
                 if (indexGeom) {
                     osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
                     for (Index corner = 0; corner < numCorners; corner ++)
                         corners->push_back(cl[corner]);
                     geom->addPrimitiveSet(corners.get());
                 } else if (numCorners > 0){
                     geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, numCorners));
                 } else {
                     geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, numVertices));
                 }
             } else {
                 Index numCorners = bin.ncl.size();
                 if (indexGeom) {
                     cl = bin.ncl.data();
                     osg::ref_ptr<osg::DrawElementsUInt> corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
                     for (Index corner = 0; corner < numCorners; corner ++)
                         corners->push_back(cl[corner]);
                     geom->addPrimitiveSet(corners.get());
                 } else if (numCorners > 0){
                     geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, numCorners));
                 } else {
                     geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.prim.size()*3));
                 }
             }

             osg::ref_ptr<osg::Vec3Array> norm = applyTriangle<Normals::const_ptr, osg::Vec3Array, true>(triangles, normals, indexGeom, bin);
             if (!norm)
                 norm = applyTriangle<osg::Vec3Array*, osg::Vec3Array, false>(triangles, gnormals, indexGeom, bin);
             geom->setNormalArray(norm.get());

             auto tc = applyTriangle<vistle::Texture1D::const_ptr, osg::FloatArray, false>(triangles, tex, indexGeom, bin);
             if (tc) {
                 tc->setBinding(osg::Array::BIND_PER_VERTEX);
                 geom->setTexCoordArray(0, tc);
             }

             bin.clear();
         }

         state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
         break;
      }

      case vistle::Object::LINES: {
         indexGeom = false;

         vistle::Lines::const_ptr lines = vistle::Lines::as(m_geo);
         const Index numElements = lines->getNumElements();
         const Index numCorners = lines->getNumCorners();

         debug << "Lines: [ #c " << numCorners << ", #e " << numElements << " ]";

         const Index *el = &lines->el()[0];
         const Index *cl = &lines->cl()[0];
         const vistle::Scalar *x = &lines->x()[0];
         const vistle::Scalar *y = &lines->y()[0];
         const vistle::Scalar *z = &lines->z()[0];

         auto geom = new osg::Geometry();
         draw.push_back(geom);
         osg::ref_ptr<osg::DrawArrayLengths> primitives =
            new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

         osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

         for (Index index = 0; index < numElements; index ++) {

            Index start = el[index];
            Index end = el[index+1];
            Index num = end - start;

            primitives->push_back(num);

            for (Index n = 0; n < num; n ++) {
               Index v = cl[start + n];
               vertices->push_back(osg::Vec3(x[v], y[v], z[v]));
            }
         }

         if (m_ro->hasSolidColor) {
             const auto &c = m_ro->solidColor;
             osg::Vec4Array *colArray = new osg::Vec4Array();
             colArray->push_back(osg::Vec4(c[0], c[1], c[2], c[3]));
             colArray->setBinding(osg::Array::BIND_OVERALL);
             geom->setColorArray(colArray);
         }

         state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
         state->setAttributeAndModes(new osg::LineWidth(2.f), osg::StateAttribute::ON);

         geom->setVertexArray(vertices.get());
         geom->addPrimitiveSet(primitives.get());
         break;
      }

      case vistle::Object::POLYGONS: {
         indexGeom = false;

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

         auto geom = new osg::Geometry();
         draw.push_back(geom);
         osg::ref_ptr<osg::DrawArrayLengths> primitives = new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON);

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

         norm->setBinding(osg::Array::BIND_PER_VERTEX);
         geom->setNormalArray(norm.get());

         break;
      }

      default:
         assert(isSupported(m_geo->getType()) == false);
         break;
   }

   // set shader parameters
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
                       debug << "ignoring " << keyvalue << ": no '=' sign" << std::endl;
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

   for (auto d: draw) {
       if (!d.get())
           continue;
       d->setName(nodename+".draw");
       //d->setStateSet(state.get());

#ifdef COVER_PLUGIN
       opencover::cover->setRenderStrategy(d.get());
       if (auto geom = d->asGeometry()) {
           s_coverMutex.lock();
           s_kdtree->apply(*geom);
           s_coverMutex.unlock();
       }
#endif

       geode->setStateSet(state.get());
       geode->addDrawable(d);
   }

   vistle::Coords::const_ptr coords = vistle::Coords::as(m_geo);
   vistle::Indexed::const_ptr indexed = vistle::Indexed::as(m_geo);
   vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);
   vistle::Spheres::const_ptr spheres = vistle::Spheres::as(m_geo);
#ifdef COVER_PLUGIN
   if (spheres && sphere) {
       if(vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_tex)) {
           vistle::DataBase::Mapping mapping = tex->guessMapping();
           if (mapping == vistle::DataBase::Vertex) {
               const auto numCoords = spheres->getNumCoords();
               auto pix = tex->pixels().data();
               std::vector<float> rgba[4];
               for (int c=0; c<4; ++c)
                   rgba[c].resize(numCoords);
               for (Index index = 0; index < numCoords; ++index) {
                   Index tc = tex->coords()[index];
                   Vector4 col = Vector4(pix[tc*4], pix[tc*4+1], pix[tc*4+2], pix[tc*4+3]);
                   for (int c=0; c<4; ++c)
                       rgba[c][index] = col[c];
                   for (int c=0; c<4; ++c)
                       rgba[c][index] = c<=1 ? 1.f : 0.f;
               }
               sphere->setColorBinding(opencover::Bind::PerVertex);
               sphere->updateColors(rgba[0].data(), rgba[1].data(), rgba[2].data(), rgba[3].data());
           } else {
               std::cerr << "VistleGeometryGenerator: Spheres: texture size mismatch, expected: " << coords->getNumCoords() << ", have: " << tex->getNumCoords() << std::endl;
               debug << "VistleGeometryGenerator: Spheres: texture size mismatch, expected: " << coords->getNumCoords() << ", have: " << tex->getNumCoords() << std::endl;
           }
       }
   } else
#endif
   if (coords) {
       osg::Geometry *geom = nullptr;
       if (!draw.empty())
           geom = draw[0]->asGeometry();
       if (vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_tex)) {
           vistle::DataBase::Mapping mapping = tex->guessMapping();
           if (mapping == vistle::DataBase::Unspecified) {
               std::cerr << "VistleGeometryGenerator: Coords: texture size mismatch, expected: " << coords->getNumCoords() << ", have: " << tex->getNumCoords() << std::endl;
               debug << "VistleGeometryGenerator: Coords: texture size mismatch, expected: " << coords->getNumCoords() << ", have: " << tex->getNumCoords() << std::endl;
           } else if (!tex->coords()) {
               std::cerr << "VistleGeometryGenerator: invalid Texture1D: no coords" << std::endl;
               debug << "VistleGeometryGenerator: invalid Texture1D: no coords" << std::endl;
           } else {
               osg::ref_ptr<osg::Texture1D> osgTex = new osg::Texture1D;
               osgTex->setName(nodename+".tex");
               osgTex->setDataVariance(osg::Object::DYNAMIC);

               osg::ref_ptr<osg::Image> image = new osg::Image();
               image->setName(nodename+".img");
               image->setImage(tex->getWidth(), 1, 1, GL_RGBA, GL_RGBA,
                               GL_UNSIGNED_BYTE, &tex->pixels()[0],
                       osg::Image::NO_DELETE);
               osgTex->setImage(image);
               osg::ref_ptr<osg::FloatArray> tc;
               if (!triangles) {
                   tc = new osg::FloatArray;
                   if (mapping == vistle::DataBase::Vertex) {
                       const Index *cl = nullptr;
                       if (indexed && indexed->getNumCorners() > 0)
                           cl =  &indexed->cl()[0];
                       if (indexGeom || !cl)
                       {
                           const auto numCoords = coords->getNumCoords();
                           const auto ntc = tex->getNumCoords();
                           if (numCoords == ntc) {
                               for (Index index = 0; index < numCoords; ++index) {
                                   tc->push_back(tex->coords()[index]);
                               }
                           } else {
                               std::cerr << "VistleGeometryGenerator: Texture1D mismatch: #coord=" << numCoords << ", #texcoord=" << ntc << std::endl;
                               debug << "VistleGeometryGenerator: Texture1D mismatch: #coord=" << numCoords << ", #texcoord=" << ntc << std::endl;
                           }
                       }
                       else if (indexed && cl) {
                           const auto el = &indexed->el()[0];
                           const auto numElements = indexed->getNumElements();
                           for (Index index = 0; index < numElements; ++index) {
                               const Index num = el[index + 1] - el[index];
                               for (Index n = 0; n < num; n ++) {
                                   Index v = cl[el[index] + n];
                                   tc->push_back(tex->coords()[v]);
                               }
                           }
                       }
                   } else if (mapping == vistle::DataBase::Element) {
                       if (indexed) {
                           const auto el = &indexed->el()[0];
                           const auto numElements = indexed->getNumElements();
                           for (Index index = 0; index < numElements; ++index) {
                               const Index num = el[index + 1] - el[index];
                               for (Index n = 0; n < num; n ++) {
                                   Index v = el[index];
                                   tc->push_back(tex->coords()[v]);
                               }
                           }
                       } else {
                           const auto numCoords = coords->getNumCoords();
                           for (Index index = 0; index < numCoords; ++index) {
                               tc->push_back(tex->coords()[index]);
                           }
                       }
                   } else {
                       std::cerr << "VistleGeometryGenerator: unknown data mapping " << mapping << " for Texture1D" << std::endl;
                       debug << "VistleGeometryGenerator: unknown data mapping " << mapping << " for Texture1D" << std::endl;
                   }
                   if (!tc->empty()) {
                       geom->setTexCoordArray(0, tc);
                   }
               }
               if (tc || (tex && triangles)) {
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

   return transform;
}
