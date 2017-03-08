#ifndef LEVELLER_H
#define LEVELLER_H

#include <vector>
#include <core/index.h>
#include <core/vec.h>
#include <core/scalar.h>
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
   vistle::UnstructuredGrid::const_ptr m_grid;
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

   template<class Data, class pol>
   vistle::Index calculateSurface(Data &data);

public:
   Leveller(const IsoController &isocontrol, vistle::UnstructuredGrid::const_ptr grid, const vistle::Scalar isovalue, vistle::Index processortype);

   bool process();
#ifndef CUTTINGSURFACE
   void setIsoData(vistle::Vec<vistle::Scalar>::const_ptr obj);
#endif
   void addMappedData(vistle::DataBase::const_ptr mapobj );
   vistle::Object::ptr result();
   vistle::DataBase::ptr mapresult() const;
   vistle::DataBase::ptr cellresult() const;
   std::pair<vistle::Scalar, vistle::Scalar> range();
};

#endif
