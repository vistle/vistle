#ifndef LEVELLER_H
#define LEVELLER_H

#include <vector>
#include <core/index.h>
#include <core/vec.h>
#include <core/scalar.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>
#include <core/unstr.h>
#include <core/triangles.h>
#include <util/enum.h>

#include "IsoDataFunctor.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ThrustBackend,
                                    (Host)
                                    (Device)
)

class Leveller  {

   const IsoController &m_isocontrol;
   vistle::Object::const_ptr m_grid;
   vistle::UniformGrid::const_ptr m_uni;
   vistle::RectilinearGrid::const_ptr m_rect;
   vistle::StructuredGrid::const_ptr m_str;
   vistle::UnstructuredGrid::const_ptr m_unstr;
   vistle::StructuredGridBase::const_ptr m_strbase;
   vistle::Coords::const_ptr m_coord;
#ifndef CUTTINGSURFACE
   vistle::Vec<vistle::Scalar>::const_ptr m_data;
#endif
   std::vector<vistle::Object::const_ptr> m_vertexdata;
   std::vector<vistle::DataBase::const_ptr> m_celldata;
   vistle::Scalar m_isoValue;
   vistle::Index m_processortype;
   vistle::Triangles::ptr m_triangles;
   std::vector<vistle::DataBase::ptr> m_outvertData;
   std::vector<vistle::DataBase::ptr> m_outcellData;
   vistle::Scalar gmin, gmax;
   vistle::Matrix4 m_objectTransform;
   bool m_computeNormals;

   template<class Data, class pol>
   vistle::Index calculateSurface(Data &data);

public:
   Leveller(const IsoController &isocontrol, vistle::Object::const_ptr grid, const vistle::Scalar isovalue, vistle::Index processortype);
   void setComputeNormals(bool value);
   void addMappedData(vistle::DataBase::const_ptr mapobj );

   bool process();
#ifndef CUTTINGSURFACE
   void setIsoData(vistle::Vec<vistle::Scalar>::const_ptr obj);
#endif
   vistle::Object::ptr result();
   vistle::DataBase::ptr mapresult() const;
   vistle::DataBase::ptr cellresult() const;
   std::pair<vistle::Scalar, vistle::Scalar> range();
};

#endif
