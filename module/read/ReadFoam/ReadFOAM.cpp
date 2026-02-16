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

#include "ReadFOAM.h"
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/core/message.h>

//Includes copied from covise ReadFOAM.cpp
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cctype>

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

#include "foamtoolbox.h"
#include <vistle/util/coRestraint.h>
#include <vistle/util/filesystem.h>
#include <boost/serialization/vector.hpp>
#include <boost/mpi.hpp>
#include <unordered_set>
namespace mpi = boost::mpi;

using namespace vistle;

static const std::string Invalid = Reader::InvalidChoice;

ReadFOAM::ReadFOAM(const std::string &name, int moduleId, mpi::communicator comm)
: Reader(name, moduleId, comm), m_boundOut(nullptr)
{
    // all ranks have to be scheduled simultaneously
    Reader::setHandlePartitions(Reader::Monolithic);
    Reader::setCollectiveIo(Reader::Collective);

    // file browser parameter
    m_casedir =
        addStringParameter("casedir", "OpenFOAM case directory", "/data/OpenFOAM", Parameter::ExistingDirectory);
    m_foamRunDir = addIntParameter("foam_case", "select a case to set it's directory as casedir", 0, Parameter::Choice);
    auto foamRunDir = getenv("FOAM_RUN");

    if (foamRunDir)
        setFoamRunDir(foamRunDir);
    //Time Parameters
    m_starttime = addFloatParameter("starttime", "start reading at the first step after this time", 0.);
    setParameterMinimum<Float>(m_starttime, 0.);
    m_stoptime = addFloatParameter("stoptime", "stop reading at the last step before this time",
                                   std::numeric_limits<double>::max());
    setParameterMinimum<Float>(m_stoptime, 0.);

    for (int i = 0; i < NumPorts; ++i) {
        { // Data Ports
            std::stringstream s;
            s << "data_out" << i;
            m_volumeDataOut.push_back(createOutputPort(s.str(), "data on 3D volume"));
        }
        { // Date Choice Parameters
            std::stringstream s;
            s << "Data" << i;
            auto p = addStringParameter(s.str(), "name of field", Invalid, Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back(Invalid);
            setParameterChoices(p, choices);
            m_fieldOut.push_back(p);
        }

        linkPortAndParameter(m_volumeDataOut[i], m_fieldOut[i]);
    }
    m_boundaryPatchesAsVariants = addIntParameter(
        "patches_as_variants", "create sub-objects with variant attribute for boundary patches", 1, Parameter::Boolean);
    m_patchSelection = addStringParameter("patches", "select patches", "all", Parameter::Restraint);

    //Mesh ports
    m_boundOut = createOutputPort("grid_out1", "boundary geometry");

    for (int i = 0; i < NumBoundaryPorts; ++i) {
        { // 2d Data Ports
            std::stringstream s;
            s << "data_2d_out" << i;
            m_boundaryDataOut.push_back(createOutputPort(s.str(), "data on boundary"));
        }
        { // 2d Data Choice Parameters
            std::stringstream s;
            s << "Data2d" << i;
            auto p = addStringParameter(s.str(), "name of field", Invalid, Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back(Invalid);
            setParameterChoices(p, choices);
            m_boundaryOut.push_back(p);
        }
        linkPortAndParameter(m_boundaryDataOut[i], m_boundaryOut[i]);
    }
    m_buildGhostcellsParam = addIntParameter("build_ghostcells", "whether to build ghost cells", 1, Parameter::Boolean);
    m_buildGhost = m_buildGhostcellsParam->getValue();

    m_onlyPolyhedraParam =
        addIntParameter("only_polyhedra", "create only polyhedral cells", m_onlyPolyhedra, Parameter::Boolean);

    observeParameter(m_casedir);
    //observeParameter(m_patchSelection);
}

void ReadFOAM::setFoamRunDir(const std::string &dir)
{
    if (dir == m_foamRunBase)
        return;
    m_foamRunBase = dir;
    m_foamCaseChoices.clear();
    m_foamCaseChoices.push_back("(Select case)");
    for (auto &p: vistle::filesystem::directory_iterator(dir)) {
        try {
            if (vistle::filesystem::is_directory(p)) {
                m_foamCaseChoices.push_back(p.path().filename().string());
            }
        } catch (const vistle::filesystem::filesystem_error &e) {
            if (e.code() == std::errc::permission_denied)
                continue;
            std::cerr << e.what() << '\n';
        }
    }
    setParameterChoices(m_foamRunDir, m_foamCaseChoices);
}


std::vector<std::string> ReadFOAM::getFieldList() const
{
    std::vector<std::string> choices;
    choices.push_back(Invalid);

    if (m_case.valid) {
        for (auto &field: m_case.constantFields)
            choices.push_back(field.first);
        for (auto &field: m_case.varyingFields)
            choices.push_back(field.first);
    }

    return choices;
}

int ReadFOAM::rankForBlock(int processor) const
{
    if (m_case.numblocks == 0)
        return 0;

    if (processor == -1)
        return -1;

    return processor % size();
}

bool ReadFOAM::examine(const Parameter *p)
{
    if (!p || p == m_casedir || p == m_foamRunDir) {
        const std::string casedir = m_casedir->getValue();
        setFoamRunDir(m_casedir->isDefault() ? casedir : vistle::filesystem::path(casedir).parent_path().string());

        if (m_case.valid && m_case.casedir == casedir)
            return true;

        m_case = getCaseInfo(casedir);
        if (!m_case.valid) {
            std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
            return false;
        }
        if (rank() == 0) {
            std::cerr << "OpenFOAM case: " << m_case.casedir << std::endl;
            std::cerr << "# processors: " << m_case.numblocks << std::endl;
            std::cerr << "# time steps: " << m_case.timedirs.size() << std::endl;
            std::cerr << "grid topology: " << (m_case.varyingGrid ? "varying" : "constant") << std::endl;
            std::cerr << "grid coordinates: " << (m_case.varyingCoords ? "varying" : "constant") << std::endl;
        }

        setTimesteps(m_case.timedirs.size());

        //print out a list of boundary patches to Vistle Console
        if (rank() == 0) {
            Boundaries bounds = m_case.loadBoundary("constant/polyMesh");
            if (bounds.valid) {
                sendInfo("boundary patches:");
                for (Index i = 0; i < bounds.boundaries.size(); ++i) {
                    std::stringstream info;
                    info << bounds.boundaries[i].index << " ## " << bounds.boundaries[i].name;
                    sendInfo("%s", info.str().c_str());
                }
            } else {
                sendInfo("No global boundary file was found at %s",
                         (m_case.casedir + "/constant/polyMesh/boundary").c_str());
            }
        }

        //fill choice parameters
        std::vector<std::string> choices = getFieldList();
        for (auto out: m_fieldOut) {
            setParameterChoices(out, choices);
        }
        for (auto out: m_boundaryOut) {
            setParameterChoices(out, choices);
        }
    }

    return true;
}

bool ReadFOAM::changeParameter(const Parameter *param)
{
    if (param == m_foamRunDir) {
        int caseNum = m_foamRunDir->getValue();
        if (caseNum == 0)
            return Reader::changeParameter(param);
        auto sep = vistle::filesystem::path::preferred_separator;
        vistle::filesystem::path foam_run_path(m_foamRunBase);
        auto foamRunDir = foam_run_path / m_foamCaseChoices[caseNum];
        setParameter(m_casedir, foamRunDir.string());
        setParameter(m_foamRunDir, vistle::Integer{0});
    }
    return Reader::changeParameter(param);
}

bool ReadFOAM::read(Reader::Token &token, int time, int part)
{
    const std::string casedir = m_casedir->getValue();

    assert(part == -1);
    (void)part;

    if (time == -1) {
        return readConstant(token, casedir);
    }

    return readTime(token, casedir, token.meta().timeStep());
}

bool ReadFOAM::prepareRead()
{
    const std::string casedir = m_casedir->getValue();
    m_boundaryPatches.add(m_patchSelection->getValue());
    if (!m_case.valid)
        m_case = getCaseInfo(casedir);
    if (!m_case.valid) {
        std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
        return false;
    }

    m_buildGhost = m_buildGhostcellsParam->getValue() && m_case.numblocks > 0;
    m_onlyPolyhedra = m_onlyPolyhedraParam->getValue();
    m_readGrid = false;
    for (auto p: m_volumeDataOut) {
        if (isConnected(*p)) {
            m_readGrid = true;
            break;
        }
    }
    m_readBoundary = isConnected(*m_boundOut);
    for (auto p: m_boundaryDataOut) {
        if (isConnected(*p)) {
            m_readBoundary = true;
            break;
        }
    }

    if (rank() == 0) {
        std::cerr << "# processors: " << m_case.numblocks << std::endl;
        std::cerr << "# time steps: " << m_case.timedirs.size() << std::endl;
        std::cerr << "grid topology: " << (m_case.varyingGrid ? "varying" : "constant") << std::endl;
        std::cerr << "grid coordinates: " << (m_case.varyingCoords ? "varying" : "constant") << std::endl;
        std::cerr << "read grid: " << (m_readGrid ? "yes" : "no") << std::endl;
        std::cerr << "read boundary: " << (m_readBoundary ? "yes" : "no") << std::endl;
    }

    return true;
}

bool ReadFOAM::finishRead()
{
    assert(m_requests.empty());
    assert(m_GhostCellsOut.empty());
    assert(m_GhostDataOut.empty());
    assert(m_GhostCellsIn.empty());
    assert(m_GhostDataIn.empty());

    m_currentbound.clear();
    m_currentgrid.clear();
    m_basedir.clear();
    m_currentvolumedata.clear();
    m_owners.clear();
    m_boundaries.clear();
    m_procBoundaryVertices.clear();
    m_procGhostCellCandidates.clear();
    m_verticesMappings.clear();

    return true;
}

bool ReadFOAM::loadCoords(const std::string &meshdir, Coords::ptr grid)
{
    std::shared_ptr<std::istream> pointsIn = m_case.getStreamForFile(meshdir, "points");
    if (!pointsIn)
        return false;
    HeaderInfo pointsH = readFoamHeader(*pointsIn);
    grid->setSize(pointsH.lines);
    if (!readFloatVectorArray(pointsH, *pointsIn, grid->x().data(), grid->y().data(), grid->z().data(),
                              pointsH.lines)) {
        std::cerr << "readFloatVectorArray for " << meshdir << "/points failed" << std::endl;
        return false;
    }
    return true;
}

GridDataContainer ReadFOAM::loadGrid(const std::string &meshdir, std::string topologyDir)
{
    std::cerr << "loadGrid('" << meshdir << "', '" << topologyDir << "')" << std::endl;
    if (topologyDir.empty())
        topologyDir = meshdir;
    bool patchesAsVariants = m_boundaryPatchesAsVariants->getValue();

    std::shared_ptr<Boundaries> boundaries(new Boundaries());
    *boundaries = m_case.loadBoundary(topologyDir);
    UnstructuredGrid::ptr grid(new UnstructuredGrid(0, 0, 0));
    std::vector<Polygons::ptr> polyList;
    size_t numPatches = 0;
    for (const auto &b: boundaries->boundaries) {
        int boundaryIndex = b.index;
        if (b.numFaces > 0 && m_boundaryPatches(boundaryIndex)) {
            ++numPatches;
        }
    }
    for (size_t i = 0; i < (patchesAsVariants ? numPatches : 1); ++i) {
        polyList.emplace_back(new Polygons(0, 0, 0));
    }
    std::shared_ptr<std::vector<Index>> owners(new std::vector<Index>());
    GridDataContainer result(grid, polyList, owners, boundaries);
    if (!m_readGrid && !m_readBoundary) {
        return result;
    }

    //read mesh files
    std::shared_ptr<std::istream> ownersIn = m_case.getStreamForFile(topologyDir, "owner");
    if (!ownersIn)
        return result;
    HeaderInfo ownerH = readFoamHeader(*ownersIn);
    DimensionInfo dim = parseDimensions(ownerH.note);
    owners->resize(ownerH.lines);
    if (!readIndexArray(ownerH, *ownersIn, (*owners).data(), (*owners).size())) {
        std::cerr << "readIndexArray for " << topologyDir << "/owner failed" << std::endl;
        return result;
    }

    {
        std::shared_ptr<std::istream> facesIn = m_case.getStreamForFile(topologyDir, "faces");
        if (!facesIn)
            return result;
        HeaderInfo facesH = readFoamHeader(*facesIn);
        std::vector<std::vector<Index>> faces(facesH.lines);
        if (!readIndexListArray(facesH, *facesIn, faces.data(), faces.size())) {
            std::cerr << "readIndexListArray for " << topologyDir << "/faces failed" << std::endl;
            return result;
        }

        std::shared_ptr<std::istream> neighboursIn = m_case.getStreamForFile(topologyDir, "neighbour");
        if (!neighboursIn)
            return result;
        HeaderInfo neighbourH = readFoamHeader(*neighboursIn);
        if (neighbourH.lines != dim.internalFaces) {
            std::cerr << "inconsistency: #internalFaces != #neighbours (" << dim.internalFaces
                      << " != " << neighbourH.lines << ")" << std::endl;
            return result;
        }
        std::vector<Index> neighbours(neighbourH.lines);
        if (!readIndexArray(neighbourH, *neighboursIn, neighbours.data(), neighbours.size()))
            return result;

        //Boundary Polygon
        if (m_readBoundary) {
            if (patchesAsVariants) {
                size_t boundIdx = 0;
                for (const auto &b: boundaries->boundaries) {
                    int boundaryIndex = b.index;
                    if (b.numFaces > 0 && m_boundaryPatches(boundaryIndex)) {
                        const auto &poly = polyList[boundIdx];
                        auto &polys = poly->el();
                        auto &conn = poly->cl();
                        polys.reserve(b.numFaces + 1);

                        for (index_t i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                            auto &face = faces[i];
                            for (Index j = 0; j < face.size(); ++j) {
                                conn.push_back(face[j]);
                            }
                            polys.push_back(conn.size());
                        }
                        ++boundIdx;
                    }
                }
            } else {
                Index num_bound = 0;
                for (const auto &b: boundaries->boundaries) {
                    int boundaryIndex = b.index;
                    if (m_boundaryPatches(boundaryIndex)) {
                        num_bound += b.numFaces;
                    }
                }
                const auto &poly = polyList[0];
                auto &polys = poly->el();
                auto &conn = poly->cl();
                polys.reserve(num_bound + 1);
                for (const auto &b: boundaries->boundaries) {
                    int boundaryIndex = b.index;
                    if (m_boundaryPatches(boundaryIndex) && b.numFaces > 0) {
                        for (index_t i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                            auto &face = faces[i];
                            for (Index j = 0; j < face.size(); ++j) {
                                conn.push_back(face[j]);
                            }
                            polys.push_back(conn.size());
                        }
                    }
                }
            }
        }

        //Grid
        if (m_readGrid) {
            grid->el().resize(dim.cells + 1);
            grid->tl().resize(dim.cells);
            //Create CellFaceMap
            std::vector<std::vector<Index>> cellfacemap(dim.cells);
            for (Index face = 0; face < (*owners).size(); ++face) {
                cellfacemap[(*owners)[face]].push_back(face);
            }
            for (Index face = 0; face < neighbours.size(); ++face) {
                cellfacemap[neighbours[face]].push_back(face);
            }

            //Vertices lists for GhostCell creation -
            //each node creates lists of the outer vertices that are shared with other domains
            //with either its own or the neighbouring domain's face-numbering (clockwise or ccw)
            //so that two domains have the same list for a mutual border
            //therefore m_procBoundaryVertices[0][1] point to the same vertices in the same order as
            //m_procBoundaryVertices[1][0] (though each list may use different labels for each vertex)
            if (m_buildGhost) {
                for (const auto &b: boundaries->procboundaries) {
                    std::vector<Index> outerVertices;
                    int myProc = b.myProc;
                    int neighborProc = b.neighborProc;
                    if (myProc < neighborProc) {
                        //create with own numbering
                        for (Index i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                            auto face = faces[i];
                            for (Index j = 0; j < face.size(); ++j) {
                                outerVertices.push_back(face[j]);
                            }
                        }
                    } else {
                        //create with neighbour numbering (reverse direction)
                        for (Index i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                            auto face = faces[i];
                            outerVertices.push_back(face[0]);
                            for (Index j = face.size() - 1; j > 0; --j) {
                                outerVertices.push_back(face[j]);
                            }
                        }
                    }

                    //check for ghost cells recursively
                    std::unordered_set<Index> ghostCellCandidates;
                    std::unordered_set<Index> notGhostCells;
                    for (Index i = 0; i < b.numFaces; ++i) {
                        Index cell = (*owners)[b.startFace + i];
                        ghostCellCandidates.insert(cell);
                    }
                    //               std::sort(ghostCellCandidates.begin(),ghostCellCandidates.end()); //Sort Vector3 by ascending Value
                    //               ghostCellCandidates.erase(std::unique(ghostCellCandidates.begin(), ghostCellCandidates.end()), ghostCellCandidates.end()); //Delete duplicate entries
                    for (Index i = 0; i < b.numFaces; ++i) {
                        Index cell = (*owners)[b.startFace + i];
                        std::vector<Index> adjacentCells =
                            getAdjacentCells(cell, dim, cellfacemap, *owners, neighbours);
                        for (Index j = 0; j < adjacentCells.size(); ++j) {
                            if (!checkCell(adjacentCells[j], ghostCellCandidates, notGhostCells, dim, outerVertices,
                                           cellfacemap, faces, *owners, neighbours))
                                std::cerr << "ERROR finding GhostCellCandidates" << std::endl;
                        }
                    }
                    m_procGhostCellCandidates[myProc][neighborProc] = ghostCellCandidates;
                    m_procBoundaryVertices[myProc][neighborProc] = outerVertices;
                }
            }

            auto tl = grid->tl().data();
            Index num_conn = 0;
            //Check Shape of Cells and fill Type_List
            for (index_t i = 0; i < dim.cells; i++) {
                const std::vector<Index> &cellfaces = cellfacemap[i];

                bool onlySimpleFaces = !m_onlyPolyhedra; // only faces with 3 or 4 corners
                int threeVert = 0, fourVert = 0;
                for (index_t j = 0; j < cellfaces.size(); ++j) {
                    if (faces[cellfaces[j]].size() == 4) {
                        ++fourVert;
                        if (fourVert > 6)
                            break;
                    } else if (faces[cellfaces[j]].size() == 3) {
                        ++threeVert;
                        if (threeVert > 4)
                            break;
                    } else {
                        onlySimpleFaces = false;
                        break;
                    }
                }
                const Index num_faces = cellfaces.size();
                Index num_verts = 0;
                if (num_faces == 6 && fourVert == 6 && threeVert == 0 && onlySimpleFaces) {
                    tl[i] = UnstructuredGrid::HEXAHEDRON;
                    num_verts = 8;
                } else if (num_faces == 5 && fourVert == 3 && threeVert == 2 && onlySimpleFaces) {
                    tl[i] = UnstructuredGrid::PRISM;
                    num_verts = 6;
                } else if (num_faces == 5 && fourVert == 1 && threeVert == 4 && onlySimpleFaces) {
                    tl[i] = UnstructuredGrid::PYRAMID;
                    num_verts = 5;
                } else if (num_faces == 4 && fourVert == 0 && threeVert == 4 && onlySimpleFaces) {
                    tl[i] = UnstructuredGrid::TETRAHEDRON;
                    num_verts = 4;
                } else {
                    tl[i] = UnstructuredGrid::POLYHEDRON;
                    for (Index j = 0; j < cellfaces.size(); ++j) {
                        num_verts += faces[cellfaces[j]].size() + 1;
                    }
                }
                num_conn += num_verts;
            }
            //save data cell by cell to element, connectivity and type list
            auto el = grid->el().data();
            auto &connectivities = grid->cl();
            auto inserter = std::back_inserter(connectivities);
            connectivities.reserve(num_conn);
            auto insertPoly = [&cellfacemap, &faces, &connectivities, &inserter, &owners, &neighbours, &dim](Index i) {
                const auto &cellfaces = cellfacemap[i]; //get all faces of current cell
                for (index_t j = 0; j < cellfaces.size(); j++) {
                    index_t ia = cellfaces[j];
                    const auto &a = faces[ia];

                    if (!isPointingInwards(ia, i, dim.internalFaces, (*owners), neighbours)) {
                        std::copy(a.rbegin(), a.rend(), inserter);
                        connectivities.push_back(*a.rbegin());
                    } else {
                        std::copy(a.begin(), a.end(), inserter);
                        connectivities.push_back(*a.begin());
                    }
                }
            };
#define FINDVERTEX_OR_POLY(idx) \
    { \
        auto v = findVertexAlongEdge(a[idx], ia, cellfaces, faces); \
        if (v != -1) { \
            a.push_back(v); \
        } else { \
            std::cerr << "ERROR: findVertexAlongEdge failed for " << a[idx] << std::endl; \
            tl[i] = UnstructuredGrid::POLYGON; \
            insertPoly(i); \
            break; \
        } \
    }
            for (index_t i = 0; i < dim.cells; i++) {
                //element list
                el[i] = connectivities.size();
                //connectivity list
                const auto &cellfaces = cellfacemap[i]; //get all faces of current cell
                switch (tl[i]) {
                case UnstructuredGrid::HEXAHEDRON: {
                    index_t ia = cellfaces[0]; //choose the first face as starting face
                    std::vector<index_t> a = faces[ia];

                    //vistle requires that the vertices-numbering of the first face conforms with the right hand rule (pointing into the cell)
                    //so the starting face is tested if it does and the numbering is reversed if it doesn't
                    if (!isPointingInwards(ia, i, dim.internalFaces, (*owners), neighbours)) {
                        std::reverse(a.begin(), a.end());
                    }

                    // bottom face
                    const auto b = a;
#if 1
                    for (auto v: b) {
                        bool found = false;
                        for (int f = 1; f < 6; ++f) {
                            const auto &face = faces[cellfaces[f]];
                            for (int i = 0; i < 4; ++i) {
                                if (face[i] == v) {
                                    found = true;
                                    const int next = (i + 1) % 4;
                                    auto it = std::find(b.begin(), b.end(), face[next]);
                                    if (it == b.end()) {
                                        a.push_back(face[next]);
                                    } else {
                                        const int prev = (i + 4 - 1) % 4;
                                        a.push_back(face[prev]);
                                    }
                                    break;
                                }
                            }
                            if (found)
                                break;
                        }
                        assert(found);
                    }
#else
                    FINDVERTEX_OR_POLY(0);
                    FINDVERTEX_OR_POLY(1);
                    FINDVERTEX_OR_POLY(2);
                    FINDVERTEX_OR_POLY(3);
#endif
                    std::copy(a.begin(), a.end(), inserter);
                } break;

                case UnstructuredGrid::PRISM: {
                    index_t it = 1;
                    index_t ia = cellfaces[0];
                    while (faces[ia].size() > 3) { //find first face with 3 vertices to use as starting face
                        ia = cellfaces[it++];
                    }

                    std::vector<index_t> a = faces[ia];

                    if (!isPointingInwards(ia, i, dim.internalFaces, (*owners), neighbours)) {
                        std::reverse(a.begin(), a.end());
                    }

                    FINDVERTEX_OR_POLY(0);
                    FINDVERTEX_OR_POLY(1);
                    FINDVERTEX_OR_POLY(2);
                    std::copy(a.begin(), a.end(), inserter);
                } break;

                case UnstructuredGrid::PYRAMID: {
                    index_t it = 1;
                    index_t ia = cellfaces[0];
                    while (faces[ia].size() < 4) { //find the rectangular face to use as starting face
                        ia = cellfaces[it++];
                    }

                    std::vector<index_t> a = faces[ia];

                    if (!isPointingInwards(ia, i, dim.internalFaces, (*owners), neighbours)) {
                        std::reverse(a.begin(), a.end());
                    }

                    FINDVERTEX_OR_POLY(0);
                    std::copy(a.begin(), a.end(), inserter);
                } break;

                case UnstructuredGrid::TETRAHEDRON: {
                    index_t ia = cellfaces[0]; //use first face as starting face
                    std::vector<index_t> a = faces[ia];

                    if (!isPointingInwards(ia, i, dim.internalFaces, (*owners), neighbours)) {
                        std::reverse(a.begin(), a.end());
                    }

                    FINDVERTEX_OR_POLY(0);
                    std::copy(a.begin(), a.end(), inserter);
                } break;

                case UnstructuredGrid::POLYHEDRON: {
                    insertPoly(i);
                } break;

                default: {
                    std::cerr << "cell #" << i << " has invalid type 0x" << std::hex << tl[i] << std::dec << std::endl;
                    break;
                }
                }
            }
            el[dim.cells] = connectivities.size();
        }
    }

    if (m_readGrid) {
        loadCoords(meshdir, grid);
    }

    if (m_readBoundary) {
        if (m_readGrid) {
            //if grid has been read already and boundary polygons are read also -> re-use coordinate lists for the boundary-polygon
            for (auto &poly: polyList) {
                poly->d()->x[0] = grid->d()->x[0];
                poly->d()->x[1] = grid->d()->x[1];
                poly->d()->x[2] = grid->d()->x[2];
            }
        } else {
            //else read coordinate lists just for boundary polygons
            bool first = true;
            for (auto &poly: polyList) {
                if (first) {
                    loadCoords(meshdir, poly);
                } else {
                    poly->d()->x[0] = polyList[0]->d()->x[0];
                    poly->d()->x[1] = polyList[0]->d()->x[1];
                    poly->d()->x[2] = polyList[0]->d()->x[2];
                }
            }
        }
    }
    return result;
}

DataBase::ptr ReadFOAM::loadField(const std::string &meshdir, const std::string &field)
{
    std::shared_ptr<std::istream> stream = m_case.getStreamForFile(meshdir, field);
    if (!stream) {
        std::cerr << "failed to open " << meshdir << "/" << field << std::endl;
        return DataBase::ptr();
    }
    HeaderInfo header = readFoamHeader(*stream);
    if (header.fieldclass == "volScalarField") {
        Vec<Scalar>::ptr s(new Vec<Scalar>(header.lines));
        if (!readFloatArray(header, *stream, s->x().data(), s->x().size())) {
            std::cerr << "readFloatArray for " << meshdir << "/" << field << " failed" << std::endl;
            return DataBase::ptr();
        }
        return s;
    } else if (header.fieldclass == "volVectorField") {
        Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(header.lines));
        if (!readFloatVectorArray(header, *stream, v->x().data(), v->y().data(), v->z().data(), v->x().size())) {
            std::cerr << "readFloatVectorArray for " << meshdir << "/" << field << " failed" << std::endl;
            return DataBase::ptr();
        }
        return v;
    }

    std::cerr << "cannot interpret " << meshdir << "/" << field << std::endl;
    return DataBase::ptr();
}

std::vector<DataBase::ptr> ReadFOAM::loadBoundaryField(const std::string &meshdir, const std::string &field,
                                                       const int &processor)
{
    bool asVariants = m_boundaryPatchesAsVariants->getValue();
    auto &boundaries = *m_boundaries[processor];
    auto owners = m_owners[processor]->data();

    //Create the dataMapping Vector3
    std::vector<std::string> patchNames;
    std::vector<std::vector<index_t>> dataMapping;
    if (asVariants) {
        size_t idx = 0;
        for (const auto &b: boundaries.boundaries) {
            Index boundaryIndex = b.index;
            if (b.numFaces > 0 && m_boundaryPatches(boundaryIndex)) {
                dataMapping.resize(idx + 1);
                for (index_t i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                    dataMapping[idx].push_back(owners[i]);
                }
                patchNames.emplace_back(b.name);
                ++idx;
            }
        }
    } else {
        dataMapping.resize(1);
        for (const auto &b: boundaries.boundaries) {
            Index boundaryIndex = b.index;
            if (b.numFaces > 0 && m_boundaryPatches(boundaryIndex)) {
                for (index_t i = b.startFace; i < b.startFace + b.numFaces; ++i) {
                    dataMapping[0].push_back(owners[i]);
                }
            }
        }
    }

    std::shared_ptr<std::istream> stream = m_case.getStreamForFile(meshdir, field);
    if (!stream) {
        std::cerr << "failed to open " << meshdir << "/" << field << std::endl;
    }
    HeaderInfo header = readFoamHeader(*stream);
    std::vector<scalar_t> fullX(header.lines), fullY(header.lines), fullZ(header.lines);
    if (header.fieldclass == "volScalarField") {
        fullX.resize(header.lines);
        if (!readFloatArray(header, *stream, fullX.data(), header.lines)) {
            std::cerr << "readFloatArray for " << meshdir << "/" << field << " failed" << std::endl;
        }
    } else if (header.fieldclass == "volVectorField") {
        fullX.resize(header.lines);
        fullY.resize(header.lines);
        fullZ.resize(header.lines);
        if (!readFloatVectorArray(header, *stream, fullX.data(), fullY.data(), fullZ.data(), header.lines)) {
            std::cerr << "readFloatVectorArray for " << meshdir << "/" << field << " failed" << std::endl;
        }
    } else {
        std::cerr << "cannot interpret " << meshdir << "/" << field << std::endl;
    }

    std::vector<DataBase::ptr> result;

    for (size_t idx = 0; idx < dataMapping.size(); ++idx) {
        if (header.fieldclass == "volScalarField") {
            Vec<Scalar>::ptr s(new Vec<Scalar>(dataMapping[idx].size()));
            auto x = s->x().data();
            for (index_t i = 0; i < dataMapping[idx].size(); ++i) {
                x[i] = fullX[dataMapping[idx][i]];
            }
            if (asVariants)
                s->addAttribute(attribute::Variant, patchNames[idx]);
            updateMeta(s);
            result.push_back(s);

        } else if (header.fieldclass == "volVectorField") {
            Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(dataMapping[idx].size()));
            auto x = v->x().data();
            auto y = v->y().data();
            auto z = v->z().data();
            for (index_t i = 0; i < dataMapping[idx].size(); ++i) {
                x[i] = fullX[dataMapping[idx][i]];
                y[i] = fullY[dataMapping[idx][i]];
                z[i] = fullZ[dataMapping[idx][i]];
            }
            if (asVariants)
                v->addAttribute(attribute::Variant, patchNames[idx]);
            updateMeta(v);
            result.push_back(v);
        }
    }
    return result;
}

void ReadFOAM::setMeta(Reader::Token &token, Object::ptr obj, int processor, int timestep) const
{
    if (obj) {
        Index skipfactor = timeIncrement();
        obj->setTimestep(timestep);
        obj->setNumTimesteps((m_case.timedirs.size() + skipfactor - 1) / skipfactor);
        token.applyMeta(obj);
        obj->setBlock(processor == -1 ? 0 : processor);
        obj->setNumBlocks(m_case.numblocks == 0 ? 1 : m_case.numblocks);

        if (timestep >= 0) {
            int i = 0;
            for (auto &ts: m_case.timedirs) {
                if (i == timestep) {
                    obj->setRealTime(ts.first);
                    break;
                }
                ++i;
            }
        }
    }
}

bool ReadFOAM::loadFields(Reader::Token &token, const std::string &meshdir, const std::map<std::string, int> &fields,
                          int processor, int timestep)
{
    for (int i = 0; i < NumPorts; ++i) {
        if (!m_volumeDataOut[i]->isConnected())
            continue;
        std::string field = m_fieldOut[i]->getValue();
        auto it = fields.find(field);
        if (it == fields.end())
            continue;
        DataBase::ptr obj = loadField(meshdir, field);
        if (obj) {
            setMeta(token, obj, processor, timestep);
            obj->describe(field, id());
        }
        m_currentvolumedata[processor][i] = obj;
    }

    size_t numFields = 0;
    bool canAdd = true;
    std::vector<std::vector<DataBase::ptr>> portFields(NumBoundaryPorts);
    for (int i = 0; i < NumBoundaryPorts; ++i) {
        if (!m_boundaryDataOut[i]->isConnected())
            continue;
        std::string field = m_boundaryOut[i]->getValue();
        auto it = fields.find(field);
        if (it == fields.end())
            continue;
        portFields[i] = loadBoundaryField(meshdir, field, processor);
        auto &fields = portFields[i];
        if (numFields == 0) {
            numFields = fields.size();
        } else if (numFields != fields.size()) {
            std::cerr << "MISMATCH: trying to load field " << field << " in " << meshdir << " on proc " << processor
                      << " for t=" << timestep << std::endl;
            std::cerr << "MISMATCH: on boundary port " << i << ", fields.size()=" << fields.size() << ", but expect "
                      << numFields << std::endl;
            canAdd = false;
        }
        if (fields.size() != m_currentbound[processor].size()) {
            std::cerr << "MISMATCH: trying to load field " << field << " in " << meshdir << " on proc " << processor
                      << " for t=" << timestep << std::endl;
            std::cerr << "MISMATCH: fields.size()=" << fields.size()
                      << ", curbound[proc].size()=" << m_currentbound[processor].size() << std::endl;
            canAdd = false;
        }
        assert(fields.size() == m_currentbound[processor].size());
    }
    if (!canAdd)
        return true;

    for (int i = 0; i < NumBoundaryPorts; ++i) {
        if (!m_boundaryDataOut[i]->isConnected())
            continue;
        std::string field = m_boundaryOut[i]->getValue();
        auto &fields = portFields[i];
        for (size_t j = 0; j < fields.size(); ++j) {
            auto &obj = fields[j];
            assert(obj);
            if (obj) {
                setMeta(token, obj, processor, timestep);
                obj->describe(field, id());
                obj->setMapping(DataBase::Element);
                obj->setGrid(m_currentbound[processor][j]);
                addObject(m_boundaryDataOut[i], obj);
            }
        }
    }
    return true;
}


bool ReadFOAM::readDirectory(Reader::Token &token, const std::string &casedir, int processor, int timestep)
{
    std::string dir;

    if (processor >= 0) {
        std::stringstream s;
        s << "processor" << processor << "/";
        dir += s.str();
    }

    if (timestep < 0) {
        dir += m_case.constantdir + "/";
        if (!m_case.varyingGrid) {
            auto ret = loadGrid(dir + "polyMesh");
            UnstructuredGrid::ptr grid = ret.grid;
            setMeta(token, grid, processor, timestep);
            for (auto &poly: ret.polygon)
                setMeta(token, poly, processor, timestep);
            m_owners[processor] = ret.owners;
            m_boundaries[processor] = ret.boundaries;

            m_currentgrid[processor] = grid;
            m_currentbound[processor] = ret.polygon;
            m_basedir[processor] = dir;
        }
        loadFields(token, dir, m_case.constantFields, processor, timestep);
        return true;
    }

    Index i = 0;
    Index skipfactor = timeIncrement();
    std::string completeMeshDir;
    for (auto &ts: m_case.timedirs) {
        if (i == timestep * skipfactor) {
            completeMeshDir = dir;
            auto it = m_case.completeMeshDirs.find(ts.first);
            if (it != m_case.completeMeshDirs.end()) {
                completeMeshDir = dir + it->second + "/";
            }
            dir += ts.second + "/";
            break;
        }
        ++i;
    }
    if (i == m_case.timedirs.size()) {
        std::cerr << "no directory for timestep " << timestep << " found" << std::endl;
        return false;
    }
    if (m_case.varyingGrid || m_case.varyingCoords) {
        UnstructuredGrid::ptr grid;
        std::vector<Polygons::ptr> polygons;
        if (completeMeshDir == m_basedir[processor]) {
            {
                grid.reset(new UnstructuredGrid(0, 0, 0));
                UnstructuredGrid::Data *od = m_currentgrid[processor]->d();
                UnstructuredGrid::Data *nd = grid->d();
                nd->tl = od->tl;
                nd->el = od->el;
                nd->cl = od->cl;
            }
            loadCoords(dir + "polyMesh", grid);
            {
                for (size_t j = 0; j < m_currentbound[processor].size(); ++j) {
                    Polygons::ptr poly(new Polygons(0, 0, 0));
                    Polygons::Data *od = m_currentbound[processor][j]->d();
                    Polygons::Data *nd = poly->d();
                    nd->el = od->el;
                    nd->cl = od->cl;
                    for (int i = 0; i < 3; ++i)
                        poly->d()->x[i] = grid->d()->x[i];
                    polygons.push_back(poly);
                }
            }
        } else {
            auto ret = loadGrid(dir + "polyMesh", completeMeshDir + "polyMesh");
            grid = ret.grid;
            polygons = ret.polygon;
            m_owners[processor] = ret.owners;
            m_boundaries[processor] = ret.boundaries;
            m_basedir[processor] = completeMeshDir;
        }
        setMeta(token, grid, processor, timestep);
        for (auto &poly: polygons)
            setMeta(token, poly, processor, timestep);
        m_currentgrid[processor] = grid;
        m_currentbound[processor] = polygons;
    }
    loadFields(token, dir, m_case.varyingFields, processor, timestep);

    return true;
}

int tag(int p, int n, int i = 0)
{ //MPI needs a unique ID for each pair of send/receive request, this function creates unique ids for each processor pairing
    return p * 10000 + n * 100 + i;
}

std::vector<Index> ReadFOAM::getAdjacentCells(const Index &cell, const DimensionInfo &dim,
                                              const std::vector<std::vector<Index>> &cellfacemap,
                                              const std::vector<Index> &owners, const std::vector<Index> &neighbours)
{
    const std::vector<Index> &cellfaces = cellfacemap[cell];
    std::vector<Index> adjacentCells;
    for (Index i = 0; i < cellfaces.size(); ++i) {
        if (cellfaces[i] < dim.internalFaces) {
            Index adjCell;
            Index o = owners[cellfaces[i]];
            Index n = neighbours[cellfaces[i]];
            adjCell = (o == cell ? n : o);
            adjacentCells.push_back(adjCell);
        }
    }
    return adjacentCells;
}

bool ReadFOAM::checkCell(const Index cell, std::unordered_set<Index> &ghostCellCandidates,
                         std::unordered_set<Index> &notGhostCells, const DimensionInfo &dim,
                         const std::vector<Index> &outerVertices, const std::vector<std::vector<Index>> &cellfacemap,
                         const std::vector<std::vector<Index>> &faces, const std::vector<Index> &owners,
                         const std::vector<Index> &neighbours)
{
    if (notGhostCells.count(cell) == 1) { // if cell is already known to not be a ghost-cell
        return true;
    }

    if (ghostCellCandidates.count(cell) == 1) { // if cell is not an already known ghost-cell
        return true;
    }

    bool isGhostCell = false;
    const std::vector<Index> &cellfaces = cellfacemap[cell];
    const auto cellvertices = getVerticesForCell(cellfaces, faces);
    for (auto it = cellvertices.begin(); it != cellvertices.end();
         ++it) { //check if the cell has a vertex on the outer boundary
        if (std::find(outerVertices.begin(), outerVertices.end(), *it) != outerVertices.end()) {
            isGhostCell = true;
            break;
        }
    }

    if (isGhostCell) {
        ghostCellCandidates.insert(cell);
        std::vector<Index> adjacentCells = getAdjacentCells(cell, dim, cellfacemap, owners, neighbours);
        for (Index i: adjacentCells) {
            if (!checkCell(i, ghostCellCandidates, notGhostCells, dim, outerVertices, cellfacemap, faces, owners,
                           neighbours))
                return false;
        }
        return true;
    } else {
        notGhostCells.insert(cell);
        return true;
    }

    return false;
}

bool ReadFOAM::buildGhostCells(int processor, GhostMode mode)
{
    //std::cerr << "buildGhostCells(p=" << processor << ", mode=" << mode << ")" << std::endl;
    auto &boundaries = *m_boundaries[processor];

    UnstructuredGrid::ptr grid = m_currentgrid[processor];
    const auto el = grid->el().data();
    const auto cl = grid->cl().data();
    const auto tl = grid->tl().data();
    const auto x = grid->x().data();
    const auto y = grid->y().data();
    const auto z = grid->z().data();

    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        std::shared_ptr<GhostCells> out(new GhostCells()); //object that will be sent to neighbor processor
        m_GhostCellsOut[processor][neighborProc] = out;
        std::vector<Index> &procBoundaryVertices = m_procBoundaryVertices[processor][neighborProc];
        std::unordered_set<Index> &procGhostCellCandidates = m_procGhostCellCandidates[processor][neighborProc];

        std::vector<Index> &elOut = out->el;
        std::vector<SIndex> &clOut = out->cl;
        std::vector<Byte> &tlOut = out->tl;
        std::vector<Scalar> &pointsOutX = out->x;
        std::vector<Scalar> &pointsOutY = out->y;
        std::vector<Scalar> &pointsOutZ = out->z;

        Index coordCount = 0;
        if (mode == ALL || mode == BASE) { //create ghost cell topology and send vertice-coordinates
            //build ghost cell element list and connectivity list for current boundary patch
            elOut.push_back(0);
            //Create Vertices Mapping
            std::map<Index, SIndex> verticesMapping;
            //shared vertices (coords do not have to be sent) -> mapped to negative values
            SIndex c = -1;
            for (const Index v: procBoundaryVertices) {
                if (verticesMapping.emplace(v, c)
                        .second) { //emplace tries to create the map entry with the key/value pair (v,c) - if an entry with the key v  already exists it does not insert the new one
                    --c; //and returns a pair which consosts of a pointer to the already existing key/value pair (first) and a boolean that states if anything was inserted into the map (second)
                }
            }
            auto mapIndex = [&verticesMapping, &coordCount](SIndex point) -> SIndex {
                //vertices with coordinates that have to be sent -> mapped to positive values
                if (verticesMapping.emplace(point, coordCount).second) {
                    ++coordCount;
                }
                return verticesMapping[point];
            };
            for (const Index cell: procGhostCellCandidates) {
                Index elementStart = el[cell];
                Index elementEnd = el[cell + 1];
                // also POLYHEDRON
                for (Index j = elementStart; j < elementEnd; ++j) {
                    clOut.push_back(mapIndex(cl[j]));
                }
                elOut.push_back(clOut.size());
                tlOut.push_back(tl[cell]);
            }

            //save the vertices mapping for later use
            m_verticesMappings[processor][neighborProc] = verticesMapping;
        } else if (mode == COORDS) {
            const std::map<Index, SIndex> &verticesMapping = m_verticesMappings[processor][neighborProc];
            for (const auto &v: verticesMapping) {
                SIndex s = v.second;
                if (s >= 0) {
                    ++coordCount;
                }
            }
        }

        if (mode == ALL || mode == COORDS) {
            //create and fill Coordinate Vectors that have to be sent
            pointsOutX.resize(coordCount);
            pointsOutY.resize(coordCount);
            pointsOutZ.resize(coordCount);

            const std::map<Index, SIndex> &verticesMapping = m_verticesMappings[processor][neighborProc];
            for (const auto &v: verticesMapping) {
                Index f = v.first;
                SIndex s = v.second;
                if (s >= 0) {
                    pointsOutX[s] = x[f];
                    pointsOutY[s] = y[f];
                    pointsOutZ[s] = z[f];
                }
            }
        }
    }

    //build requests for Ghost Cells
    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        int myRank = rank();
        int neighborRank = rankForBlock(neighborProc);
        std::shared_ptr<GhostCells> out = m_GhostCellsOut[processor][neighborProc];
        if (myRank != neighborRank) {
            m_requests.push_back(comm().isend(neighborRank, tag(processor, neighborProc), *out));
            std::shared_ptr<GhostCells> in(new GhostCells());
            m_GhostCellsIn[processor][neighborProc] = in;
            m_requests.push_back(comm().irecv(neighborRank, tag(neighborProc, processor), *in));
        } else {
            m_GhostCellsIn[neighborProc][processor] = out;
        }
    }
    return true;
}

bool ReadFOAM::buildGhostCellData(int processor)
{
    //std::cerr << "buildGhostCellsData(p=" << processor << ")" << std::endl;
    assert(m_buildGhost);
    auto &boundaries = *m_boundaries[processor];
    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        std::unordered_set<Index> &procGhostCellCandidates = m_procGhostCellCandidates[processor][neighborProc];
        for (int i = 0; i < NumPorts; ++i) {
            auto f = m_currentvolumedata[processor].find(i);
            if (f == m_currentvolumedata[processor].end()) {
                continue;
            }
            Object::ptr obj = f->second;
            Vec<Scalar, 1>::ptr v1 = Vec<Scalar, 1>::as(obj);
            Vec<Scalar, 3>::ptr v3 = Vec<Scalar, 3>::as(obj);
            if (!v1 && !v3) {
                std::cerr << "Could not send Data - unsupported Data-Object Type" << std::endl;
                continue;
            }
            if (v1) {
                std::shared_ptr<GhostData> dataOut(new GhostData(1));
                m_GhostDataOut[processor][neighborProc][i] = dataOut;
                auto &d = v1->x(0);
                for (const Index &cell: procGhostCellCandidates) {
                    (*dataOut).x[0].push_back(d[cell]);
                }
            } else if (v3) {
                std::shared_ptr<GhostData> dataOut(new GhostData(3));
                m_GhostDataOut[processor][neighborProc][i] = dataOut;
                auto &d1 = v3->x(0);
                auto &d2 = v3->x(1);
                auto &d3 = v3->x(2);
                for (const Index &cell: procGhostCellCandidates) {
                    (*dataOut).x[0].push_back(d1[cell]);
                    (*dataOut).x[1].push_back(d2[cell]);
                    (*dataOut).x[2].push_back(d3[cell]);
                }
            }
        }
    }

    //build requests for Ghost Data
    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        int myRank = rank();
        int neighborRank = rankForBlock(neighborProc);

        std::map<int, std::shared_ptr<GhostData>> &m = m_GhostDataOut[processor][neighborProc];
        for (Index i = 0; i < NumPorts; ++i) {
            if (m.find(i) != m.end()) {
                std::shared_ptr<GhostData> dataOut = m[i];
                if (myRank != neighborRank) {
                    m_requests.push_back(comm().isend(neighborRank, tag(processor, neighborProc, i + 1), *dataOut));
                    std::shared_ptr<GhostData> dataIn(new GhostData((*dataOut).dim));
                    m_GhostDataIn[processor][neighborProc][i] = dataIn;
                    m_requests.push_back(comm().irecv(neighborRank, tag(neighborProc, processor, i + 1), *dataIn));
                } else {
                    m_GhostDataIn[neighborProc][processor][i] = dataOut;
                }
            }
        }
    }
    return true;
}

void ReadFOAM::processAllRequests()
{
    assert(m_buildGhost);
    mpi::wait_all(m_requests.begin(), m_requests.end());
    m_requests.clear();
    m_GhostCellsOut.clear();
    m_GhostDataOut.clear();
}

void ReadFOAM::applyGhostCells(int processor, GhostMode mode)
{
    assert(m_buildGhost);
    auto &boundaries = *m_boundaries[processor];
    UnstructuredGrid::ptr grid = m_currentgrid[processor];
    auto &el = grid->el();
    auto &cl = grid->cl();
    auto &tl = grid->tl();
    auto &ghost = grid->ghost();
    auto &x = grid->x();
    auto &y = grid->y();
    auto &z = grid->z();
    ghost.resize(tl.size(), cell::NORMAL);
    //std::cerr << "applyGhostCells(p=" << processor << ", mode=" << mode << "), #cells=" << el.size() << ", #coords: " << x.size() << std::endl;

    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        std::vector<Index> &procBoundaryVertices = m_procBoundaryVertices[processor][neighborProc];
        std::vector<Index> sharedVerticesMapping;
        for (const Index &v: procBoundaryVertices) {
            // create sharedVerticesMapping vector that consists of all the shared (already known) vertices
            // without duplicates and in order of "first appearance" when going through the boundary cells
            if (std::find(sharedVerticesMapping.begin(), sharedVerticesMapping.end(), v) ==
                sharedVerticesMapping.end()) {
                sharedVerticesMapping.push_back(v);
            }
        }
        std::shared_ptr<GhostCells> in = m_GhostCellsIn[processor][neighborProc];
        std::vector<Index> &elIn = in->el;
        std::vector<SIndex> &clIn = in->cl;
        std::vector<Byte> &tlIn = in->tl;
        std::vector<Scalar> &pointsInX = in->x;
        std::vector<Scalar> &pointsInY = in->y;
        std::vector<Scalar> &pointsInZ = in->z;
        Index pointsSize = x.size();
        if (pointsSize == 0) {
            std::cerr << "Warning: no coordinates loaded yet" << std::endl;
        }

        if (mode == ALL ||
            mode == BASE) { //ghost cell topology is unknown and has to be appended to the current topology
            for (Index cell = 0; cell < tlIn.size(); ++cell) { //append new topology to old grid
                Index elementStart = elIn[cell];
                Index elementEnd = elIn[cell + 1];
                auto mapIndex = [sharedVerticesMapping, pointsSize](SIndex point) -> SIndex {
                    if (point <
                        0) { //if point<0 then vertex is already known and can be looked up in sharedVerticesMapping
                        return sharedVerticesMapping[-point - 1];
                    } else { //else the vertex is unknown and its coordinates will be appended (in order of first appearance) to the old coord-lists so we point to an index beyond the current size
                        return point + pointsSize;
                    }
                };
                // also POLYHEDRON
                for (Index i = elementStart; i < elementEnd; ++i) {
                    cl.push_back(mapIndex(clIn[i]));
                }
                el.push_back(cl.size());
                tl.push_back(tlIn[cell]);
                ghost.push_back(cell::GHOST);
                assert(tl.size() == ghost.size());
            }
        }

        if (mode == ALL || mode == COORDS) {
            //append new coordinates to old coordinate-lists
            std::copy(pointsInX.begin(), pointsInX.end(), std::back_inserter(x));
            std::copy(pointsInY.begin(), pointsInY.end(), std::back_inserter(y));
            std::copy(pointsInZ.begin(), pointsInZ.end(), std::back_inserter(z));
        }
    }
    m_GhostCellsIn[processor].clear();
    m_procBoundaryVertices[processor].clear();
    //std::cerr << "applyGhostCells(p=" << processor << ", mode=" << mode << "), final #cells=" << el.size() << ", #coords: " << x.size() << std::endl;
}

void ReadFOAM::applyGhostCellsData(int processor)
{
    //std::cerr << "applyGhostCellsData(p=" << processor << ")" << std::endl;
    auto &boundaries = *m_boundaries[processor];

    for (const auto &b: boundaries.procboundaries) {
        Index neighborProc = b.neighborProc;
        for (int i = 0; i < NumPorts; ++i) {
            auto f = m_currentvolumedata[processor].find(i);
            if (f == m_currentvolumedata[processor].end()) {
                continue;
            }
            Object::ptr obj = f->second;
            Vec<Scalar, 1>::ptr v1 = Vec<Scalar, 1>::as(obj);
            Vec<Scalar, 3>::ptr v3 = Vec<Scalar, 3>::as(obj);
            if (!v1 && !v3) {
                std::cerr << "Could not apply Data - unsupported Data-Object Type" << std::endl;
                continue;
            }
            //append ghost cell data to old data objects
            if (v1) {
                std::shared_ptr<GhostData> dataIn = m_GhostDataIn[processor][neighborProc][i];
                auto &d = v1->x(0);
                std::vector<Scalar> &x = (*dataIn).x[0];
                std::copy(x.begin(), x.end(), std::back_inserter(d));
            } else if (v3) {
                std::shared_ptr<GhostData> dataIn = m_GhostDataIn[processor][neighborProc][i];
                for (Index j = 0; j < 3; ++j) {
                    auto &d = v3->x(j);
                    std::vector<Scalar> &x = (*dataIn).x[j];
                    std::copy(x.begin(), x.end(), std::back_inserter(d));
                }
            }
        }
    }
    m_GhostDataIn[processor].clear();
}

bool ReadFOAM::addBoundaryToPort(Reader::Token &token, int processor)
{
    for (auto &poly: m_currentbound[processor])
        if (poly) {
            addObject(m_boundOut, poly);
        }
    return true;
}

bool ReadFOAM::addVolumeDataToPorts(Reader::Token &token, int processor)
{
    std::vector<DataBase::ptr> portData(NumPorts);
    bool canAdd = true;
    auto &volumedata = m_currentvolumedata[processor];
    for (int portnum = 0; portnum < NumPorts; ++portnum) {
        if (!m_volumeDataOut[portnum]->isConnected())
            continue;
        auto it = volumedata.find(portnum);
        if (it != volumedata.end()) {
            const auto &obj = it->second;
            if (obj) {
                obj->setMapping(DataBase::Element);
                obj->setGrid(m_currentgrid[processor]);
                updateMeta(obj);
                portData[portnum] = obj;
            } else {
                canAdd = false;
            }
        } else {
            if (m_currentgrid[processor])
                portData[portnum] = m_currentgrid[processor];
            else
                canAdd = false;
        }
    }
    if (!canAdd) {
        std::cerr << "CANNOT ADD" << std::endl;
    }
    if (canAdd) {
        for (int portnum = 0; portnum < NumPorts; ++portnum) {
            if (!m_volumeDataOut[portnum]->isConnected())
                continue;
            addObject(m_volumeDataOut[portnum], portData[portnum]);
        }
    }
    return true;
}

bool ReadFOAM::readConstant(Reader::Token &token, const std::string &casedir)
{
    for (int i = -1; i < m_case.numblocks; ++i) {
        if (rankForBlock(i) == rank()) {
            if (!readDirectory(token, casedir, i, -1))
                return false;
        }
    }

    if (m_case.varyingCoords && m_case.varyingGrid)
        return true;

    if (m_readBoundary && !m_case.varyingCoords) {
        for (int i = -1; i < m_case.numblocks; ++i) {
            if (rankForBlock(i) == rank()) {
                addBoundaryToPort(token, i);
            }
        }
    }

    if (!m_readGrid)
        return true;

    GhostMode buildGhostMode = m_case.varyingCoords ? BASE : ALL;

    if (m_buildGhost) {
        for (int i = 0; i < m_case.numblocks; ++i) {
            if (rankForBlock(i) == rank()) {
                if (!buildGhostCells(i, buildGhostMode))
                    return false;
            }
        }

        processAllRequests();
    }

    for (int i = -1; i < m_case.numblocks; ++i) {
        if (rankForBlock(i) == rank()) {
            if (m_buildGhost) {
                applyGhostCells(i, buildGhostMode);
            }
        }
    }

    m_GhostCellsIn.clear();
    m_currentvolumedata.clear();
    return true;
}

bool ReadFOAM::readTime(Reader::Token &token, const std::string &casedir, int timestep)
{
    for (int i = -1; i < m_case.numblocks; ++i) {
        if (rankForBlock(i) == rank()) {
            if (!readDirectory(token, casedir, i, timestep))
                return false;
        }
    }

    if (!m_readGrid)
        return true;

    if (m_case.varyingCoords || m_case.varyingGrid) {
        const GhostMode ghostMode = m_case.varyingGrid ? ALL : COORDS;
        if (m_buildGhost) {
            for (int i = 0; i < m_case.numblocks; ++i) {
                if (rankForBlock(i) == rank()) {
                    if (!buildGhostCells(i, ghostMode))
                        return false;
                }
            }

            processAllRequests();
        }

        for (int i = -1; i < m_case.numblocks; ++i) {
            if (rankForBlock(i) == rank()) {
                if (m_buildGhost) {
                    applyGhostCells(i, ghostMode);
                }
                addBoundaryToPort(token, i);
            }
        }
        m_GhostCellsIn.clear();
    }

    if (m_buildGhost) {
        for (int i = 0; i < m_case.numblocks; ++i) {
            if (rankForBlock(i) == rank()) {
                buildGhostCellData(i);
            }
        }

        processAllRequests();
    }

    for (int i = -1; i < m_case.numblocks; ++i) {
        if (rankForBlock(i) == rank()) {
            if (m_buildGhost) {
                applyGhostCellsData(i);
            }
            addVolumeDataToPorts(token, i);
        }
    }

    m_GhostDataIn.clear();
    m_currentvolumedata.clear();
    return true;
}

MODULE_MAIN(ReadFOAM)
