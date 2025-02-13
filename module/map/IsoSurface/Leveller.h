#ifndef VISTLE_ISOSURFACE_LEVELLER_H
#define VISTLE_ISOSURFACE_LEVELLER_H

#include <vector>
#include <vistle/core/index.h>
#include <vistle/core/vec.h>
#include <vistle/core/scalar.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/triangles.h>
#include <vistle/core/lines.h>
#include <vistle/util/enum.h>

#ifdef CUTTINGSURFACE
#define Leveller CutLeveller
#elif defined(ISOHEIGHTSURFACE)
#define Leveller IsoHeightLeveller
#else
#define Leveller IsoLeveller
#endif

#include "IsoDataFunctor.h"

class Leveller {
    const IsoController &m_isocontrol;
    vistle::Object::const_ptr m_grid;
    vistle::UniformGrid::const_ptr m_uni;
    vistle::LayerGrid::const_ptr m_lg;
    vistle::RectilinearGrid::const_ptr m_rect;
    vistle::StructuredGrid::const_ptr m_str;
    vistle::UnstructuredGrid::const_ptr m_unstr;
    vistle::StructuredGridBase::const_ptr m_strbase;
    vistle::Polygons::const_ptr m_poly;
    vistle::Quads::const_ptr m_quad;
    vistle::Triangles::const_ptr m_tri;
    vistle::Coords::const_ptr m_coord;
#ifdef CUTTINGSURFACE
    const std::vector<vistle::Index> *m_candidateCells = nullptr;
#else
    vistle::Vec<vistle::Scalar>::const_ptr m_data;
#endif
    std::vector<vistle::Object::const_ptr> m_vertexdata;
    std::vector<vistle::DataBase::const_ptr> m_celldata;
    vistle::Scalar m_isoValue;
    vistle::Triangles::ptr m_triangles;
    vistle::Lines::ptr m_lines;
    vistle::Normals::ptr m_normals;
    std::vector<vistle::DataBase::ptr> m_outvertData;
    std::vector<vistle::DataBase::ptr> m_outcellData;
    vistle::Scalar gmin, gmax;
    vistle::Matrix4 m_objectTransform;
    bool m_computeNormals;

    template<class Data, class pol>
    vistle::Index calculateSurface(Data &data);

public:
    Leveller(const IsoController &isocontrol, vistle::Object::const_ptr grid, const vistle::Scalar isovalue);
    void setComputeNormals(bool value);
    void addMappedData(vistle::DataBase::const_ptr mapobj);

    bool process();
#ifdef CUTTINGSURFACE
    void setCandidateCells(const std::vector<vistle::Index> *candidateCells);
    const std::vector<vistle::Index> *candidateCells();
#else
    void setIsoData(vistle::Vec<vistle::Scalar>::const_ptr obj);
#endif
    vistle::Coords::ptr result();
    vistle::Normals::ptr normresult();
    vistle::DataBase::ptr mapresult() const;
    vistle::DataBase::ptr cellresult() const;
    std::pair<vistle::Scalar, vistle::Scalar> range();
};

#endif
