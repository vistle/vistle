#include <vistle/alg/objalg.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/util/stopwatch.h>
#include <vistle/vtkm/convert.h>

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/filter/contour/Contour.h>

#include <iomanip>

#include "IsoSurfaceVtkm.h"

using namespace vistle;

MODULE_MAIN(IsoSurfaceVtkm)

/*
TODO:
    - support input other than per-vertex mapped data on unstructured grid
    - add second input port from original isosurface module (mapped data)
*/

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PointOrValue, (PointPerTimestep)(Value)(PointInFirstStep))

IsoSurfaceVtkm::IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in", "input gird or geometry with scalar data");
    m_mapDataIn = createInputPort("mapdata_in", "additional mapped field");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
    m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
    m_pointOrValue = addIntParameter("point_or_value", "point or value interaction", Value, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

    setReducePolicy(message::ReducePolicy::OverAll);

    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", false, Parameter::Boolean);
}

IsoSurfaceVtkm::~IsoSurfaceVtkm()
{}

bool IsoSurfaceVtkm::changeParameter(const Parameter *param)
{
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

    return Module::changeParameter(param);
}

void IsoSurfaceVtkm::setInputSpecies(const std::string &species)
{
    m_species = species;
    Module::setInputSpecies(species);
    updateModuleInfo();
}

void IsoSurfaceVtkm::updateModuleInfo()
{
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
}

bool IsoSurfaceVtkm::prepare()
{
    m_blocksForTime.clear();

    m_min = std::numeric_limits<Scalar>::max();
    m_max = -std::numeric_limits<Scalar>::max();

    m_performedPointSearch = false;
    m_foundPoint = false;

    return Module::prepare();
}

bool IsoSurfaceVtkm::reduce(int timestep)
{
    if (rank() == 0)
        std::cerr << "IsoSurface::reduce(" << timestep << ")" << std::endl;
    Scalar value = m_isovalue->getValue();
    Vector3 point = m_isopoint->getValue();

    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<BlockData> &blocks = m_blocksForTime[timestep];
    lock.unlock();
    if ((m_pointOrValue->getValue() == PointInFirstStep && !m_performedPointSearch && timestep <= 0) ||
        m_pointOrValue->getValue() == PointPerTimestep) {
        const int numBlocks = blocks.size();
        int found = 0;
#ifdef _OPENMP
#pragma omp parallel for firstprivate(value) lastprivate(value) reduction(+ : found)
#endif
        for (int i = 0; i < numBlocks; ++i) {
            const auto &b = blocks[i];
            auto gi = b.grid->getInterface<GridInterface>();
            Index cell = gi->findCell(point, InvalidIndex, GridInterface::ForceCelltree);
            if (cell != InvalidIndex) {
                if (auto scal = Vec<Scalar>::as(b.datas)) {
                    ++found;
                    auto interpol = gi->getInterpolator(cell, point);
                    value = interpol(scal->x());
                }
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
            auto obj = work(b.grid, b.datas, b.mapdata, value);
            addObject(m_dataOut, obj);
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

    return Module::reduce(timestep);
}

int IsoSurfaceVtkm::BlockData::getTimestep() const
{
    int t = vistle::getTimestep(datas);
    if (t < 0)
        t = vistle::getTimestep(grid);
    if (t < 0)
        t = vistle::getTimestep(mapdata);
    return t;
}

Object::ptr IsoSurfaceVtkm::work(vistle::Object::const_ptr grid, vistle::DataBase::const_ptr isoField,
                                 vistle::DataBase::const_ptr mapField, Scalar isoValue) const
{
    if (auto scal = Vec<Scalar>::as(isoField)) {
        auto minmax = scal->getMinMax();
        std::lock_guard<std::mutex> guard(m_mutex);
        if (minmax.first[0] < m_min)
            m_min = minmax.first[0];
        if (minmax.second[0] > m_max)
            m_max = minmax.second[0];
    }

    // transform vistle dataset to vtkm dataset
    viskores::cont::DataSet vtkmDataSet;
    auto status = vtkmSetGrid(vtkmDataSet, grid);
    if (!status->continueExecution()) {
        sendText(status->messageType(), status->message());
        return Object::ptr();
    }

    std::string isospecies = isoField->getAttribute(attribute::Species);
    if (isospecies.empty())
        isospecies = "isodata";
    status = vtkmAddField(vtkmDataSet, isoField, isospecies);
    if (!status->continueExecution()) {
        sendText(status->messageType(), status->message());
        return Object::ptr();
    }

    std::string mapspecies;
    if (mapField) {
        mapspecies = mapField->getAttribute(attribute::Species);
        if (mapspecies.empty())
            mapspecies = "mapped";
        status = vtkmAddField(vtkmDataSet, mapField, mapspecies);
        if (!status->continueExecution()) {
            sendText(status->messageType(), status->message());
            return Object::ptr();
        }
    }

    // apply vtkm isosurface filter
    viskores::filter::contour::Contour isosurfaceFilter;
    isosurfaceFilter.SetActiveField(isospecies);
    isosurfaceFilter.SetIsoValue(isoValue);
    isosurfaceFilter.SetMergeDuplicatePoints(false);
    isosurfaceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);
    auto isosurface = isosurfaceFilter.Execute(vtkmDataSet);


    // transform result back into vistle format
    Object::ptr geoOut = vtkmGetGeometry(isosurface);
    if (!geoOut) {
        return Object::ptr();
    }

    updateMeta(geoOut);
    geoOut->copyAttributes(isoField);
    geoOut->copyAttributes(grid, false);
    geoOut->setTransform(grid->getTransform());
    if (geoOut->getTimestep() < 0) {
        geoOut->setTimestep(grid->getTimestep());
        geoOut->setNumTimesteps(grid->getNumTimesteps());
    }
    if (geoOut->getBlock() < 0) {
        geoOut->setBlock(grid->getBlock());
        geoOut->setNumBlocks(grid->getNumBlocks());
    }

    if (mapField) {
        if (auto mapOut = vtkmGetField(isosurface, mapspecies)) {
            mapOut->copyAttributes(mapField);
            mapOut->setGrid(geoOut);
            updateMeta(mapOut);
            return mapOut;
        }
    }
    return geoOut;
}

bool IsoSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto isoData = task->expect<Vec<Scalar>>("data_in");
    if (!isoData) {
        sendError("need scalar data on data_in");
        return true;
    }
    auto splitIso = splitContainerObject(isoData);
    auto grid = splitIso.geometry;
    if (!grid) {
        sendError("no grid on scalar input data");
        return true;
    }

    auto isoField = splitIso.mapped;
    if (isoField->guessMapping() != DataBase::Vertex) {
        sendError("need per-vertex mapping on data_in");
        return true;
    }

    auto mapData = task->accept<Object>(m_mapDataIn);
    auto splitMap = splitContainerObject(mapData);
    auto mapField = splitMap.mapped;
    if (mapField) {
        auto &mg = splitMap.geometry;
        if (!mg) {
            sendError("no grid on mapped data");
            return true;
        }
        if (mg->getHandle() != grid->getHandle()) {
            sendError("grids on mapped data and iso-data do not match");
            std::cerr << "grid mismatch: mapped: " << *mapField << ",\n    isodata: " << *isoField << std::endl;
            return true;
        }
    }

    if (m_pointOrValue->getValue() == Value) {
        auto obj = work(grid, isoField, mapField, m_isovalue->getValue());
        task->addObject(m_dataOut, obj);
        return true;
    } else {
        std::lock_guard<std::mutex> guard(m_mutex);
        BlockData b(grid, isoField, mapField);
        int t = b.getTimestep();
        m_blocksForTime[t].emplace_back(b);
        return true;
    }

    return true;
}
