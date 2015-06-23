#ifndef LEVELLER_H
#define LEVELLER_H

#include <vector>
#include <core/index.h>
#include <core/scalar.h>
#include <core/unstr.h>
#include <core/triangles.h>
#include <util/enum.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ThrustBackend,
                                    (Host)
                                    (Device)
)

DEFINE_ENUM_WITH_STRING_CONVERSIONS(SurfaceOption,
   (Plane)
   (CylinderX)
   (CylinderY)
   (CylinderZ)
   (Sphere)
)

class Leveller  {

   vistle::UnstructuredGrid::const_ptr m_grid;
#ifndef CUTTINGSURFACE
   vistle::Vec<vistle::Scalar>::const_ptr m_data;
#else
   vistle::Vector vertex;
   vistle::Vector point;
   vistle::Vector direction;
   int m_option;
#endif
   std::vector<vistle::Object::const_ptr> m_mapdata;
   vistle::Scalar m_isoValue;
   vistle::Index m_processortype;
   vistle::Triangles::ptr m_triangles;
   std::vector<vistle::Object::ptr> m_outmapData;
   vistle::Scalar gmin, gmax;

   template<class Data, class pol>
   vistle::Index calculateSurface(Data &data);

public:
   Leveller(vistle::UnstructuredGrid::const_ptr grid, const vistle::Scalar isovalue, vistle::Index processortype
#ifdef CUTTINGSURFACE
         , int option
#endif
         );

   bool process();
#ifdef CUTTINGSURFACE
   void setCutData(const vistle::Vector m_vertex, const vistle::Vector m_point, const vistle::Vector m_direction);
#else
   void setIsoData(vistle::Vec<vistle::Scalar>::const_ptr obj);
#endif
   void addMappedData(vistle::Object::const_ptr mapobj );
   vistle::Object::ptr result();
   vistle::Object::ptr mapresult();
   std::pair<vistle::Scalar, vistle::Scalar> range();
};

#endif
