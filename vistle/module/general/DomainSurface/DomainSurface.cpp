#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>
#include <core/unstr.h>
#include <core/polygons.h>
#include "ctime"

#include "DomainSurface.h"

using namespace vistle;

DomainSurface::DomainSurface(const std::string &shmname, const std::string &name, int moduleID)
   : Module("DomainSurface", shmname, name, moduleID) {
   createInputPort("data_in");
   createOutputPort("data_out");
   addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
   addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
   addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
   addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
   addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
   addIntParameter("reuseCoordinates", "Re-use the unstructured grids coordinate list and data-object", 0, Parameter::Boolean);
}

DomainSurface::~DomainSurface() {
}

bool DomainSurface::compute() {
   //DomainSurface Polygon
   DataBase::const_ptr data;
   m_grid_in = accept<UnstructuredGrid>("data_in");
   if (!m_grid_in) {
      data = expect<DataBase>("data_in");
      if (!data) {
          sendError("no grid and no data received");
          return true;
      }
      m_grid_in = UnstructuredGrid::as(data->grid());
      if (!m_grid_in) {
          sendError("no valid grid attached to data");
          return true;
      }
   }

   if (!createSurface())
      return true;

   m_grid_out->setMeta(m_grid_in->meta());
   m_grid_out->copyAttributes(m_grid_in);
   if (!data) {
       addObject("data_out", m_grid_out);
       return true;
   }

   if (data->getSize() != m_grid_in->getNumCoords()) {
       sendError("data size does not match grid size");
       return true;
   }

   const bool reuseCoord = getIntParameter("reuseCoordinates");
   if (reuseCoord) {
       DataBase::ptr dout = data->clone();
       dout->setGrid(m_grid_out);
       addObject("data_out", dout);
       return true;
   }

   if(auto data_in = Vec<Scalar, 3>::as(data)) {
       const Scalar *data_in_x = &data_in->x()[0];
       const Scalar *data_in_y = &data_in->y()[0];
       const Scalar *data_in_z = &data_in->z()[0];
       Vec<Scalar,3>::ptr data_obj_out(new Vec<Scalar,3>(m_verticesMapping.size()));
       Scalar *data_out_x = data_obj_out->x().data();
       Scalar *data_out_y = data_obj_out->y().data();
       Scalar *data_out_z = data_obj_out->z().data();
       for (auto &v: m_verticesMapping) {
           Index f=v.first;
           Index s=v.second;
           data_out_x[s] = data_in_x[f];
           data_out_y[s] = data_in_y[f];
           data_out_z[s] = data_in_z[f];
       }
       data_obj_out->setGrid(m_grid_out);
       data_obj_out->setMeta(data->meta());
       data_obj_out->copyAttributes(data);
       addObject("data_out", data_obj_out);
   } else if(auto data_in = Vec<Scalar, 1>::as(data)) {
       const Scalar *data_in_x = &data_in->x()[0];
       Vec<Scalar,1>::ptr data_obj_out(new Vec<Scalar,1>(m_verticesMapping.size()));
       Scalar *data_out_x = data_obj_out->x().data();
       for (auto &v: m_verticesMapping) {
           Index f=v.first;
           Index s=v.second;
           data_out_x[s] = data_in_x[f];
       }
       data_obj_out->setGrid(m_grid_out);
       data_obj_out->copyAttributes(data);
       data_obj_out->setMeta(data->meta());
       addObject("data_out", data_obj_out);
   } else {
         std::cerr << "WARNING: No valid 1D or 3D data on input Port" << std::endl;
   }

   return true;
}

bool DomainSurface::createSurface() {

   const bool showtet = getIntParameter("tetrahedron");
   const bool showpyr = getIntParameter("pyramid");
   const bool showpri = getIntParameter("prism");
   const bool showhex = getIntParameter("hexahedron");
   const bool showpol = getIntParameter("polyhedron");
   const bool reuseCoord = getIntParameter("reuseCoordinates");

   const Index num_elem = m_grid_in->getNumElements();
   const Index *el = &m_grid_in->el()[0];
   const Index *cl = &m_grid_in->cl()[0];
   const unsigned char *tl = &m_grid_in->tl()[0];
   UnstructuredGrid::VertexOwnerList::const_ptr vol=m_grid_in->getVertexOwnerList();

   m_grid_out.reset(new Polygons(0, 0, 0));
   auto &pl = m_grid_out->el();
   auto &pcl = m_grid_out->cl();

   for (Index i=0; i<num_elem; ++i) {
      unsigned char t = tl[i];
      if (t == UnstructuredGrid::POLYHEDRON) {
          if (showpol) {
              bool facecomplete=true;
              Index start=0;
              std::vector<Index> face;
              for (Index j=el[i]; j<el[i+1]; ++j) {
                  Index vertex=cl[j];
                  if (facecomplete) {
                      facecomplete=false;
                      start=vertex;
                      face.push_back(vertex);
                  } else if (vertex==start) {
                      facecomplete=true;
                      Index neighbour = vol->getNeighbour(i, face[0], face[1], face[2]);
                      if (neighbour == InvalidIndex) {
                          std::reverse(face.begin(), face.end());
                          for (const Index &v : face) {
                              pcl.push_back(v);
                          }
                          pl.push_back(pcl.size());
                      }
                      face.clear();
                  } else {
                      face.push_back(vertex);
                  }
              }
              if (!facecomplete) {
                  std::cerr << "WARNING: Polyhedron incomplete: " << i << std::endl;
              }
          }
      } else {
          bool show = false;
          switch(t) {
          case UnstructuredGrid::PYRAMID:
              show = showpyr;
              break;
          case UnstructuredGrid::PRISM:
              show = showpri;
              break;
          case UnstructuredGrid::TETRAHEDRON:
              show = showtet;
              break;
          case UnstructuredGrid::HEXAHEDRON:
              show = showhex;
              break;
          default:
              break;
          }

          if (show) {
            const auto numFaces = UnstructuredGrid::NumFaces[t];
            const auto &faces = UnstructuredGrid::FaceVertices[t];
            for (int f=0; f<numFaces; ++f) {
               const auto &face = faces[f];
               Index elStart = el[i];
               const auto facesize = UnstructuredGrid::FaceSizes[t][f];
               Index neighbour = vol->getNeighbour(i, cl[elStart + face[0]], cl[elStart + face[1]], cl[elStart + face[2]]);
               if (neighbour == InvalidIndex) {
                  for (Index j=0;j<facesize;++j) {
                     pcl.push_back(cl[elStart + face[j]]);
                  }
                  pl.push_back(pcl.size());
               }
            }
         }
      }
   }

   if (m_grid_out->getNumElements() == 0) {
      return false;
   }

   if (reuseCoord) {
      m_grid_out->d()->x[0] = m_grid_in->d()->x[0];
      m_grid_out->d()->x[1] = m_grid_in->d()->x[1];
      m_grid_out->d()->x[2] = m_grid_in->d()->x[2];
   } else {
      m_verticesMapping.clear();
      Index c=0;
      for (Index &v : m_grid_out->cl()) {
         if (m_verticesMapping.emplace(v,c).second) {
            v=c;
            ++c;
         } else {
            v=m_verticesMapping[v];
         }
      }

      const Scalar *xcoord = &m_grid_in->x()[0];
      const Scalar *ycoord = &m_grid_in->y()[0];
      const Scalar *zcoord = &m_grid_in->z()[0];
      auto &px = m_grid_out->x();
      auto &py = m_grid_out->y();
      auto &pz = m_grid_out->z();
      px.resize(c);
      py.resize(c);
      pz.resize(c);

      for (auto &v: m_verticesMapping) {
         Index f=v.first;
         Index s=v.second;
         px[s] = xcoord[f];
         py[s] = ycoord[f];
         pz[s] = zcoord[f];
      }
   }

   return true;
}

//bool DomainSurface::checkNormal(Index v1, Index v2, Index v3, Scalar x_center, Scalar y_center, Scalar z_center) {
//   Scalar *xcoord = m_grid_in->x().data();
//   Scalar *ycoord = m_grid_in->y().data();
//   Scalar *zcoord = m_grid_in->z().data();
//   Scalar a[3], b[3], c[3], n[3];

//   // compute normal of a=v2v1 and b=v2v3
//   a[0] = xcoord[v1] - xcoord[v2];
//   a[1] = ycoord[v1] - ycoord[v2];
//   a[2] = zcoord[v1] - zcoord[v2];
//   b[0] = xcoord[v3] - xcoord[v2];
//   b[1] = ycoord[v3] - ycoord[v2];
//   b[2] = zcoord[v3] - zcoord[v2];
//   n[0] = a[1] * b[2] - b[1] * a[2];
//   n[1] = a[2] * b[0] - b[2] * a[0];
//   n[2] = a[0] * b[1] - b[0] * a[1];

//   // compute vector from base-point to volume-center
//   c[0] = x_center - xcoord[v2];
//   c[1] = y_center - ycoord[v2];
//   c[2] = z_center - zcoord[v2];
//   // look if normal is correct or not
//   if ((c[0] * n[0] + c[1] * n[1] + c[2] * n[2]) > 0)
//       return false;
//   else
//       return true;
//}


MODULE_MAIN(DomainSurface)
