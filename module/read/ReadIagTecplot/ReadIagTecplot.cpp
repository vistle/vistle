#include "ReadIagTecplot.h"
#include "topo.h"
#include "mesh.h"
#include "tecplotfile.h"

#include <boost/algorithm/string/predicate.hpp>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include <vistle/util/filesystem.h>

MODULE_MAIN(ReadIagTecplot)

//using namespace vistle;
using vistle::Scalar;
using vistle::Vec;
using vistle::Index;
bool ReadIagTecplot::examine(const vistle::Parameter *param)
{
    if (param != nullptr && param != m_filename)
        return true;

    const std::string filename = m_filename->getValue();
    size_t numZones = 0;
    try {
        TecplotFile tecplot(filename);
        numZones = tecplot.NumZones();
    } catch (...) {
        std::cerr << "failed to create TecplotFile for " << filename << std::endl;
        setPartitions(0);
        return false;
    }

    setPartitions(std::min(size_t(size() * 32), numZones));

    return true;
}

bool ReadIagTecplot::read(Reader::Token &token, int timestep, int block)
{
    auto unstr = std::make_shared<vistle::UnstructuredGrid>(0, 0, 0);
    auto &x = unstr->x();
    auto &y = unstr->y();
    auto &z = unstr->z();
    auto &el = unstr->el();
    auto &cl = unstr->cl();
    auto &tl = unstr->tl();

    vistle::Vec<Scalar>::ptr p, r;
    vistle::Vec<Scalar, 3>::ptr n, u, v;

    typedef vistle::Vec<Scalar>::array array;

    array *pp = nullptr;
    if (m_p->isConnected()) {
        p = std::make_shared<Vec<vistle::Scalar>>(0);
        pp = &p->x();
    }
    array *rr = nullptr;
    if (m_rho->isConnected()) {
        r = std::make_shared<Vec<Scalar>>(0);
        rr = &r->x();
    }
    array *nx = nullptr, *ny = nullptr, *nz = nullptr;
    if (m_n->isConnected()) {
        n = std::make_shared<Vec<Scalar, 3>>(0);
        nx = &n->x();
        ny = &n->y();
        nz = &n->z();
    }
    array *ux = nullptr, *uy = nullptr, *uz = nullptr;
    if (m_u->isConnected()) {
        u = std::make_shared<Vec<Scalar, 3>>(0);
        ux = &u->x();
        uy = &u->y();
        uz = &u->z();
    }
    array *vx = nullptr, *vy = nullptr, *vz = nullptr;
    if (m_v->isConnected()) {
        v = std::make_shared<Vec<Scalar, 3>>(0);
        vx = &v->x();
        vy = &v->y();
        vz = &v->z();
    }

    const int npart = std::max(1, numPartitions());

    TecplotFile tecplot(m_filename->getValue());

    size_t numZones = tecplot.NumZones();
    size_t numZonesBlock = (numZones + npart - 1) / npart;
    size_t begin = block >= 0 ? numZonesBlock * block : 0;
    size_t end = block >= 0 ? std::min(numZones, numZonesBlock * (block + 1)) : numZones;
    std::cerr << "reading zones " << begin << " to " << end - 1 << std::endl;

    for (size_t i = 0; i < begin && i < numZones; ++i) {
        tecplot.SkipZone(i);
    }

    Index baseVertex = 0;
    for (size_t i = begin; i < end; ++i) {
        auto mesh = tecplot.ReadZone(i);
        if (auto hexmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
            hexmesh->SetupVolume();
            //std::cerr << "hexmesh: #points=" << hexmesh->getNumPoints() << ", #cells=" << hexmesh->getNumCells() << std::endl;

            std::map<int, Index> vertMap;
            for (int c = 0; c < hexmesh->getNumCells(); ++c) {
                const auto &cell = mesh->getState(c);
                const auto &hex = static_cast<const HexaederTopo &>(cell);
                bool allNodesPresent = true, allNodesBlanked = true;
                for (int n = 0; n < 8; ++n) {
                    int v = hex.mNodes[n];
                    if (v < 0) {
                        allNodesPresent = false;
                        break;
                    }
                    const auto &ps = mesh->getPointState(v);
                    if (ps.mBlank) {
                        allNodesBlanked = false;
                    }
                }
                if (!allNodesPresent)
                    continue;
                if (allNodesBlanked)
                    continue;
                for (int n = 0; n < 8; ++n) {
                    int v = hex.mNodes[n];
                    auto p = vertMap.emplace(v, baseVertex + vertMap.size());
                    if (p.second) {
                        auto &ps = mesh->getPointState(v);
                        x.push_back(ps.X());
                        y.push_back(ps.Y());
                        z.push_back(ps.Z());

                        if (pp) {
                            pp->push_back(ps.mP);
                        }
                        if (rr) {
                            rr->push_back(ps.mR);
                        }
                        if (nx && ny && nz) {
                            nx->push_back(ps.mN.X());
                            ny->push_back(ps.mN.Y());
                            nz->push_back(ps.mN.Z());
                        }
                        if (ux && uy && uz) {
                            ux->push_back(ps.mU.X());
                            uy->push_back(ps.mU.Y());
                            uz->push_back(ps.mU.Z());
                        }
                        if (vx && vy && vz) {
                            vx->push_back(ps.mV.X());
                            vy->push_back(ps.mV.Y());
                            vz->push_back(ps.mV.Z());
                        }
                    }
                    Index &vv = p.first->second;
                    cl.push_back(vv);
                }
                tl.push_back(vistle::UnstructuredGrid::HEXAHEDRON);
                el.push_back(cl.size());
            }
            baseVertex += vertMap.size();
        } else if (auto tetmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
            std::cerr << "IGNORING tetmesh: #points=" << tetmesh->getNumPoints()
                      << ", #cells=" << tetmesh->getNumCells() << std::endl;
        } else if (auto trisurf = dynamic_cast<SurfaceMesh<TriangleTopo> *>(mesh)) {
            std::cerr << "IGNORING trisurf: #points=" << trisurf->getNumPoints()
                      << ", #cells=" << trisurf->getNumCells() << std::endl;
        } else if (auto quadsurf = dynamic_cast<SurfaceMesh<QuadrangleTopo> *>(mesh)) {
            std::cerr << "IGNORING quadsurf: #points=" << quadsurf->getNumPoints()
                      << ", #cells=" << quadsurf->getNumCells() << std::endl;
        } else {
            std::cerr << "IGNORING unknown mesh type" << std::endl;
        }
        delete mesh;
    }

    token.applyMeta(unstr);
    token.addObject(m_grid, unstr);
    if (p) {
        p->describe("pressure", id());
        p->setGrid(unstr);
        token.applyMeta(p);
        token.addObject(m_p, p);
    }
    if (r) {
        r->describe("rho", id());
        r->setGrid(unstr);
        token.applyMeta(r);
        token.addObject(m_rho, r);
    }
    if (n) {
        n->describe("n", id());
        n->setGrid(unstr);
        token.applyMeta(n);
        token.addObject(m_n, n);
    }
    if (u) {
        u->describe("u", id());
        u->setGrid(unstr);
        token.applyMeta(u);
        token.addObject(m_u, u);
    }
    if (v) {
        v->describe("v", id());
        v->setGrid(unstr);
        token.applyMeta(v);
        token.addObject(m_v, v);
    }

    return true;
}


ReadIagTecplot::ReadIagTecplot(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    m_filename = addStringParameter("filename", "name of Tecplot file", "", vistle::Parameter::ExistingFilename);

    m_grid = createOutputPort("grid_out", "grid or geometry");
    m_p = createOutputPort("p", "pressure");
    m_rho = createOutputPort("rho", "rho");
    m_n = createOutputPort("n", "n");
    m_u = createOutputPort("u", "u");
    m_v = createOutputPort("v", "v");

    //setParallelizationMode(Serial);
    setParallelizationMode(ParallelizeTimeAndBlocks);

    observeParameter(m_filename);
}
