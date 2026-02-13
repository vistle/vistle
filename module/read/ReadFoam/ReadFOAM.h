#ifndef VISTLE_READFOAM_READFOAM_H
#define VISTLE_READFOAM_READFOAM_H

/**************************************************************************\
 **                                                           (C)2013 RUS  **
 **                                                                        **
 ** Description: Read FOAM data format                                     **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** History:                                                               **
 ** May   13	    C.Kopf  	    V1.0                                   **
 *\**************************************************************************/

#include <vector>
#include <map>

#include <vistle/core/polygons.h>
#include <vistle/core/unstr.h>
#include <vistle/module/reader.h>
#include <vistle/util/coRestraint.h>

#include "foamtoolbox.h"
#include <boost/mpi/request.hpp>
#include <unordered_set>

struct GridDataContainer {
    GridDataContainer(vistle::UnstructuredGrid::ptr g, std::vector<vistle::Polygons::ptr> p,
                      std::shared_ptr<std::vector<vistle::Index>> o, std::shared_ptr<Boundaries> b)
    {
        grid = g;
        polygon = p;
        owners = o;
        boundaries = b;
    }

    vistle::UnstructuredGrid::ptr grid;
    std::vector<vistle::Polygons::ptr> polygon;
    std::shared_ptr<std::vector<vistle::Index>> owners;
    std::shared_ptr<Boundaries> boundaries;
};

class GhostCells {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &el;
        ar &cl;
        ar &tl;
        ar &x;
        ar &y;
        ar &z;
    }

public:
    GhostCells() {}
    std::vector<vistle::Index> el;
    std::vector<vistle::SIndex> cl;
    std::vector<vistle::Byte> tl;
    std::vector<vistle::Scalar> x;
    std::vector<vistle::Scalar> y;
    std::vector<vistle::Scalar> z;
};

class GhostData {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &dim;
        for (int i = 0; i < dim; ++i) {
            ar &x[i];
        }
    }

public:
    GhostData(int d = 1): dim(d) {}
    std::vector<vistle::Scalar> x[vistle::MaxDimension];
    int dim;
};


class ReadFOAM: public vistle::Reader {
    static const int NumPorts = 3;
    static const int NumBoundaryPorts = 3;

public:
    enum GhostMode { ALL, BASE, COORDS };

    ReadFOAM(const std::string &name, int moduleId, mpi::communicator comm);

private:
    //Parameter
    vistle::StringParameter *m_casedir, *m_patchSelection;
    vistle::IntParameter *m_foamRunDir = nullptr;
    vistle::FloatParameter *m_starttime, *m_stoptime;
    vistle::IntParameter *m_boundaryPatchesAsVariants;
    vistle::IntParameter *m_buildGhostcellsParam;

    std::vector<std::string> m_foamCaseChoices;
    std::string m_foamRunBase;
    bool m_buildGhost;
    bool m_readGrid = false, m_readBoundary = false;
    vistle::IntParameter *m_onlyPolyhedraParam = nullptr;
    bool m_onlyPolyhedra = false;
    std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;
    //Ports
    vistle::Port *m_boundOut;
    std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

    vistle::coRestraint m_boundaryPatches;
    CaseInfo m_case;

    std::vector<std::string> getFieldList() const;

    // Reader interface
    bool examine(const vistle::Parameter *p) override;
    bool read(vistle::Reader::Token &token, int time, int part) override;
    bool prepareRead() override;
    bool finishRead() override;

    void setFoamRunDir(const std::string &dir);

    //! return MPI rank on which a block should be processed, takes OpenFOAM case, especially no. of blocks, into account
    int rankForBlock(int processor) const;
    bool readDirectory(vistle::Reader::Token &token, const std::string &dir, int processor, int timestep);
    bool buildGhostCells(int processor, GhostMode mode);
    bool buildGhostCellData(int processor);
    void processAllRequests();
    void applyGhostCells(int processor, GhostMode mode);
    void applyGhostCellsData(int processor);
    bool addBoundaryToPort(vistle::Reader::Token &token, int processor);
    bool addVolumeDataToPorts(vistle::Reader::Token &token, int processor);
    bool readConstant(vistle::Reader::Token &token, const std::string &dir);
    bool readTime(vistle::Reader::Token &token, const std::string &dir, int timestep);
    std::vector<vistle::Index> getAdjacentCells(const vistle::Index &cell, const DimensionInfo &dim,
                                                const std::vector<std::vector<vistle::Index>> &cellfacemap,
                                                const std::vector<vistle::Index> &owners,
                                                const std::vector<vistle::Index> &neighbours);
    bool checkCell(const vistle::Index cell, std::unordered_set<vistle::Index> &ghostCellCandidates,
                   std::unordered_set<vistle::Index> &notGhostCells, const DimensionInfo &dim,
                   const std::vector<vistle::Index> &outerVertices,
                   const std::vector<std::vector<vistle::Index>> &cellfacemap,
                   const std::vector<std::vector<vistle::Index>> &faces, const std::vector<vistle::Index> &owners,
                   const std::vector<vistle::Index> &neighbours);

    bool loadCoords(const std::string &meshdir, vistle::Coords::ptr grid);
    GridDataContainer loadGrid(const std::string &dir, std::string topologyDir = std::string());
    vistle::DataBase::ptr loadField(const std::string &dir, const std::string &field);
    std::vector<vistle::DataBase::ptr> loadBoundaryField(const std::string &dir, const std::string &field,
                                                         const int &processor);
    bool loadFields(vistle::Reader::Token &token, const std::string &dir, const std::map<std::string, int> &fields,
                    int processor, int timestep);

    void setMeta(vistle::Reader::Token &token, vistle::Object::ptr obj, int processor, int timestep) const;

    std::map<int, std::vector<vistle::Polygons::ptr>> m_currentbound;
    std::map<int, vistle::UnstructuredGrid::ptr> m_currentgrid;
    std::map<int, std::string> m_basedir;
    std::map<int, std::map<int, vistle::DataBase::ptr>> m_currentvolumedata;
    std::map<int, std::shared_ptr<std::vector<vistle::Index>>> m_owners;
    std::map<int, std::shared_ptr<Boundaries>> m_boundaries;
    std::map<int, std::map<int, std::vector<vistle::Index>>> m_procBoundaryVertices;
    std::map<int, std::map<int, std::unordered_set<vistle::Index>>> m_procGhostCellCandidates;
    std::map<int, std::map<int, std::shared_ptr<GhostCells>>> m_GhostCellsOut;
    std::map<int, std::map<int, std::shared_ptr<GhostCells>>> m_GhostCellsIn;
    std::map<int, std::map<int, std::map<int, std::shared_ptr<GhostData>>>> m_GhostDataOut;
    std::map<int, std::map<int, std::map<int, std::shared_ptr<GhostData>>>> m_GhostDataIn;
    std::vector<boost::mpi::request> m_requests;
    std::map<int, std::map<int, std::map<vistle::Index, vistle::SIndex>>> m_verticesMappings;
};
#endif
