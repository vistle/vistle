//
//This code is used for both IsoCut and IsoSurface!
//

#include <iostream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <boost/mpi/collectives.hpp>
#include <vistle/core/message.h>
#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/vec.h>
#include <vistle/alg/objalg.h>

#ifdef CUTTINGSURFACE
#define IsoSurface CuttingSurface
#else
#ifdef ISOHEIGHTSURFACE
#define IsoSurface IsoHeightSurface
#endif
#endif


#include "IsoSurface.h"
#include "Leveller.h"

#ifdef ISOHEIGHTSURFACE
#include "../IsoHeightSurface/MapHeight.h"
#endif

#ifdef CUTTINGSURFACE
MODULE_MAIN(CuttingSurface)
#else
#ifdef ISOHEIGHTSURFACE
MODULE_MAIN(IsoHeightSurface)
#else
MODULE_MAIN(IsoSurface)
#endif
#endif

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PointOrValue, (PointPerTimestep)(Value)(PointInFirstStep))

IsoSurface::IsoSurface(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), isocontrol(this)
{
    isocontrol.init();

    for (int i = 0; i < NumPorts; ++i) {
        std::string in = "data_in" + std::to_string(i);
        std::string out = "data_out" + std::to_string(i);
        if (i == 0)
            in = "data_in";
#ifdef ISOSURFACE
        if (i == 1)
            in = "mapdata_in";
#else
        if (i == 0)
            out = "data_out";
#endif

        m_dataIn[i] = createInputPort(in, "input data");
        m_dataOut[i] = createOutputPort(out, "output data");
        linkPorts(m_dataIn[i], m_dataOut[i]);
        if (i > 0) {
            linkPorts(m_dataIn[0], m_dataOut[i]);
            setPortOptional(m_dataIn[i], true);
        }
    }

#ifndef CUTTINGSURFACE
    setReducePolicy(message::ReducePolicy::OverAll);
#endif
#ifdef ISOHEIGHTSURFACE
    m_isovalue = addFloatParameter("iso_height", "height above ground", 0.0);
    m_heightmap = addStringParameter("heightmap", "height map as geotif", "", Parameter::ExistingFilename);
#endif
#ifdef ISOSURFACE
    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
    m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
    m_pointOrValue = addIntParameter("point_or_value", "point or value interaction", Value, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

    m_surfOut = createOutputPort("data_out", "surface without mapped data");
    linkPorts(m_dataIn[0], m_surfOut);
#endif

    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 1, Parameter::Boolean);
    m_paraMin = m_paraMax = 0.f;

#ifdef CUTTINGSURFACE
    addResultCache(m_gridCache);
#endif
}

IsoSurface::~IsoSurface() = default;

bool IsoSurface::changeParameter(const Parameter *param)
{
    bool ok = isocontrol.changeParameter(param);

#ifdef ISOSURFACE
    if (param == m_pointOrValue) {
        if (m_pointOrValue->getValue() == PointInFirstStep)
            setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
        else if (m_pointOrValue->getValue() == PointPerTimestep)
            setReducePolicy(message::ReducePolicy::PerTimestep);
        else
            setReducePolicy(message::ReducePolicy::OverAll);
    }

    if (param == m_isovalue || param == m_pointOrValue) {
        updateModuleInfo();
    }
#endif
    return Module::changeParameter(param) && ok;
}

void IsoSurface::setInputSpecies(const std::string &species)
{
    std::cerr << "Species: " << species << std::endl;
    m_species = species;
    Module::setInputSpecies(species);
    updateModuleInfo();
}

void IsoSurface::updateModuleInfo()
{
#ifdef ISOSURFACE
    if (m_pointOrValue->getValue() == PointInFirstStep || m_pointOrValue->getValue() == PointPerTimestep) {
        if (m_species.empty())
            setItemInfo("Point");
        else
            setItemInfo(m_species);
    } else {
        auto val = m_isovalue->getValue();
        std::stringstream str;
        str << std::setw(0) << std::setprecision(3) << val;
        if (m_species.empty())
            setItemInfo(str.str());
        else
            setItemInfo(m_species + "=" + str.str());
    }
#else
    setItemInfo(m_species);
#endif
}

bool IsoSurface::prepare()
{
    m_blocksForTime.clear();

    m_min = std::numeric_limits<Scalar>::max();
    m_max = -std::numeric_limits<Scalar>::max();

    m_performedPointSearch = false;
    m_foundPoint = false;

    return Module::prepare();
}

bool IsoSurface::reduce(int timestep)
{
#ifdef ISOSURFACE
    if (rank() == 0)
        std::cerr << "IsoSurface::reduce(" << timestep << ")" << std::endl;
    Scalar value = m_isovalue->getValue();
    Vector3 point = m_isopoint->getValue();

    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<BlockData> &blocks = m_blocksForTime[timestep];
    lock.unlock();
    if ((m_pointOrValue->getValue() == PointInFirstStep && !m_performedPointSearch && timestep <= 0) ||
        m_pointOrValue->getValue() == PointPerTimestep) {
        int found = 0;
        const int numBlocks = blocks.size();
#pragma omp parallel for
        for (int i = 0; i < numBlocks; ++i) {
            const auto &b = blocks[i];
            auto gi = b.grid->getInterface<GridInterface>();
            Index cell = gi->findCell(point, InvalidIndex, GridInterface::ForceCelltree);
            if (cell != InvalidIndex) {
                ++found;
                auto interpol = gi->getInterpolator(cell, point);
                value = interpol(b.datas->x());
            }
        }

        int numFound = boost::mpi::all_reduce(comm(), found, std::plus<int>());
        m_foundPoint = numFound > 0;
        if (m_rank == 0) {
            if (numFound == 0) {
                if (m_pointOrValue->getValue() != PointPerTimestep || (m_performedPointSearch && timestep != -1)) {
                    sendInfo("iso-value point out of domain for timestep %d", timestep);
                }
            } else if (numFound > 1) {
                sendWarning("found isopoint in %d blocks", numFound);
            }
        }
        int valRank = found ? m_rank : m_size;
        valRank = boost::mpi::all_reduce(comm(), valRank, boost::mpi::minimum<int>());
        if (valRank < m_size) {
            boost::mpi::broadcast(comm(), value, valRank);
            if (m_pointOrValue->getValue() == PointInFirstStep || numTimesteps() <= 1) {
                setParameter(m_isovalue, (Float)value);
            }
        }
        m_performedPointSearch = true;
    }

    if (m_foundPoint) {
        for (const auto &b: blocks) {
            auto results = work(b.grid, b.datas, b.mapdata, value);
            for (unsigned i = 0; i < results.size() && i < NumPorts; ++i) {
                const auto &obj = results[i];
                addObject(m_dataOut[i], obj);
            }
            addObject(m_surfOut, results.back());
        }
    }

    if (timestep == -1) {
        Scalar min, max;
        boost::mpi::all_reduce(comm(), m_min, min, boost::mpi::minimum<Scalar>());
        boost::mpi::all_reduce(comm(), m_max, max, boost::mpi::maximum<Scalar>());

        if (max >= min) {
            if (m_paraMin != (Float)min || m_paraMax != (Float)max)
                setParameterRange(m_isovalue, (Float)min, (Float)max);

            m_paraMax = max;
            m_paraMin = min;
        }
    }
#endif

    return Module::reduce(timestep);
}

#ifdef ISOHEIGHTSURFACE
Object::ptr IsoSurface::createHeightCut(vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar>::const_ptr dataS,
                                        vistle::DataBase::const_ptr mapdata) const
{
    Object::ptr returnObj;
    std::vector<vistle::DataBase::const_ptr> mapdataVec{mapdata};

    if (auto coordsIn = Coords::as(grid)) {
        MapHeight heightField;
        std::string mapFile = m_heightmap->getValue();
        heightField.openImage(mapFile);

        const Scalar *x = &coordsIn->x()[0], *y = &coordsIn->y()[0], *z = &coordsIn->z()[0];
        Vec<Scalar>::ptr newData(new Vec<Scalar>(coordsIn->getNumVertices()));
        auto heightMappedData = newData->x().data();

        for (Index i = 0; i < coordsIn->getNumVertices(); ++i) {
            heightMappedData[i] = z[i] - heightField.getAlt(x[i], y[i]);
        }
        heightField.closeImage();

        float val = m_isovalue->getValue();
        auto results = work(grid, newData, mapdataVec, val);
        returnObj = results[0];
    } else {
        sendInfo("No Coords object was found");
        auto results = work(grid, dataS, mapdataVec, m_isovalue->getValue());
        returnObj = results[0];
    }
    return returnObj;
}
#endif

std::vector<Object::ptr> IsoSurface::work(vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar>::const_ptr dataS,
                                          std::vector<vistle::DataBase::const_ptr> mapdata, Scalar isoValue) const
{
    Leveller l(isocontrol, grid, isoValue);
    l.setComputeNormals(m_computeNormals->getValue());

#ifdef CUTTINGSURFACE
    CachedResult cachedResult;
    auto cacheEntry = m_gridCache.getOrLock(grid->getName(), cachedResult);
    if (!cacheEntry) {
        l.setCandidateCells(cachedResult.candidateCells.get());
    }
#else
    l.setIsoData(dataS);
#endif
    for (auto &m: mapdata) {
        l.addMappedData(m);
    }
    l.process();

#ifndef CUTTINGSURFACE
    auto minmax = dataS->getMinMax();
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (minmax.first[0] < m_min)
            m_min = minmax.first[0];
        if (minmax.second[0] > m_max)
            m_max = minmax.second[0];
    }
#endif

    std::vector<Object::ptr> results;
    Coords::ptr result = l.result();
    updateMeta(result);
    Normals::ptr normals = l.normresult();
    updateMeta(normals);
    std::vector<DataBase::ptr> mapresult;
    for (int i = 0; i < NumPorts; ++i) {
        mapresult.push_back(l.mapresult(i));
        updateMeta(mapresult[i]);
    }
    if (result) {
#ifndef CUTTINGSURFACE
        result->copyAttributes(dataS);
#endif
        result->copyAttributes(grid, false);
        result->setTransform(grid->getTransform());
        result->setNormals(normals);
        if (result->getTimestep() < 0) {
            result->setTimestep(grid->getTimestep());
            result->setNumTimesteps(grid->getNumTimesteps());
        }
        if (result->getBlock() < 0) {
            result->setBlock(grid->getBlock());
            result->setNumBlocks(grid->getNumBlocks());
        }
    }
#ifdef CUTTINGSURFACE
    if (cacheEntry) {
        cachedResult.grid = result;
        cachedResult.candidateCells.reset(l.candidateCells());
        m_gridCache.storeAndUnlock(cacheEntry, cachedResult);
    }
    result = cachedResult.grid;
#endif
    for (int i = 0; i < mapresult.size(); ++i) {
        if (!result || result->isEmpty()) {
            results.push_back(Object::ptr());
            continue;
        }
        auto &m = mapresult[i];
        if (!m) {
            results.push_back(Object::ptr());
            continue;
        }
        if (mapdata[i])
            m->copyAttributes(mapdata[i]);
        m->setGrid(result);
        results.push_back(m);
    }
#ifdef ISOSURFACE
    results.push_back(result);
#endif
    return results;
}


bool IsoSurface::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto container = task->expect<DataBase>("data_in");
    auto split = splitContainerObject(container);
    auto mapdata = split.mapped;
    if (!mapdata) {
        sendError("no mapped data");
        return true;
    }
    auto grid = split.geometry;
    if (!grid) {
        sendError("no grid on input data");
        return true;
    }
#ifndef CUTTINGSURFACE
    auto dataS = Vec<Scalar>::as(split.mapped);
    if (!dataS) {
        sendError("need Scalar data on data_in");
        return true;
    }
    if (dataS->guessMapping() != DataBase::Vertex) {
        sendError("need per-vertex mapping on data_in");
        return true;
    }
#endif
    auto uni = UniformGrid::as(grid);
    auto lg = LayerGrid::as(grid);
    auto rect = RectilinearGrid::as(grid);
    auto str = StructuredGrid::as(grid);
    auto unstr = UnstructuredGrid::as(grid);
    auto tri = Triangles::as(grid);
    auto quad = Quads::as(grid);
    auto poly = Polygons::as(grid);
    if (!uni && !lg && !rect && !str && !unstr && !tri && !quad && !poly) {
        sendError("unsupported grid type on input data");
        return true;
    }

    std::vector<DataBase::const_ptr> mapdataVec{mapdata};
    for (int i = 1; i < NumPorts; ++i) {
        auto container = task->accept<Object>(m_dataIn[i]);
        auto split = splitContainerObject(container);
        if (split.geometry && grid) {
            if (split.geometry->getHandle() != grid->getHandle()) {
                sendError("grids on data received do not match");
                return true;
            }
        }
        mapdataVec.push_back(split.mapped);
    }

#ifdef CUTTINGSURFACE
    auto results = work(grid, nullptr, mapdataVec);
    for (unsigned i = 0; i < results.size() && i < NumPorts; ++i) {
        const auto &obj = results[i];
        task->addObject(m_dataOut[i], obj);
    }
    return true;
#elif defined(ISOHEIGHTSURFACE)
    std::string mapFile = m_heightmap->getValue();
    if (mapFile.empty()) {
        sendInfo("No geotiff was found for height mapping");
        return true;
    } else {
        auto obj = createHeightCut(grid, dataS, mapdata);
        task->addObject(m_dataOut[0], obj);
        return true;
    }
#else
    if (m_pointOrValue->getValue() == Value) {
        auto results = work(grid, dataS, mapdataVec, m_isovalue->getValue());
        for (unsigned i = 0; i < results.size() && i < NumPorts; ++i) {
            const auto &obj = results[i];
            task->addObject(m_dataOut[i], obj);
        }
        task->addObject(m_surfOut, results.back());
        return true;
    } else {
        std::lock_guard<std::mutex> guard(m_mutex);
        BlockData b(grid, dataS, mapdataVec);
        int t = b.getTimestep();
        m_blocksForTime[t].emplace_back(b);
        return true;
    }
#endif
}

int IsoSurface::BlockData::getTimestep() const
{
    int t = vistle::getTimestep(datas);
    if (t < 0)
        t = vistle::getTimestep(grid);
    if (t < 0)
        t = vistle::getTimestep(mapdata[0]);
    return t;
}
