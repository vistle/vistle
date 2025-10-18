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
    for (int i = 0; i < NumPorts; ++i) {
        std::string in = "data_in" + std::to_string(i);
        std::string out = "data_out" + std::to_string(i);
        if (i == 0)
            in = "data_in";
        if (i == 1)
            in = "mapdata_in";

        m_dataIn[i] = createInputPort(in, "input data");
        m_dataOut[i] = createOutputPort(out, "output data");
        linkPorts(m_dataIn[i], m_dataOut[i]);
        if (i > 0) {
            linkPorts(m_dataIn[0], m_dataOut[i]);
            setPortOptional(m_dataIn[i], true);
        }
    }
    m_surfOut = createOutputPort("data_out", "surface without mapped data");
    linkPorts(m_dataIn[0], m_surfOut);

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
    m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
    m_pointOrValue = addIntParameter("point_or_value", "point or value interaction", Value, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

    setReducePolicy(message::ReducePolicy::OverAll);

    m_computeNormals =
        addIntParameter("compute_normals", "compute per-vertex surface normals", false, Parameter::Boolean);
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
                if (auto scal = Vec<Scalar>::as(b.mapdata[0])) {
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
            auto objs = work(b.grid, b.mapdata, value);
            for (int i = 0; i < NumPorts; ++i) {
                addObject(m_dataOut[i], objs[i]);
            }
            addObject(m_surfOut, objs.back());
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
    int t = vistle::getTimestep(mapdata[0]);
    if (t < 0)
        t = vistle::getTimestep(grid);
    for (auto &m: mapdata) {
        if (t >= 0)
            break;
        t = vistle::getTimestep(m);
    }
    return t;
}

std::vector<Object::ptr> IsoSurfaceVtkm::work(vistle::Object::const_ptr grid,
                                              const std::vector<vistle::DataBase::const_ptr> &mapField,
                                              Scalar isoValue) const
{
    std::vector<Object::ptr> result(NumPorts + 1);
    auto &isoField = mapField[0];
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
        return result;
    }

    std::string isospecies = isoField->getAttribute(attribute::Species);
    if (isospecies.empty())
        isospecies = "isodata";
    status = vtkmAddField(vtkmDataSet, isoField, isospecies);
    if (!status->continueExecution()) {
        sendText(status->messageType(), status->message());
        return result;
    }

    std::vector<std::string> mapspecies;
    int unnamed = 0;
    for (auto &m: mapField) {
        std::string sp;
        if (m) {
            sp = m->getAttribute(attribute::Species);
            if (sp.empty()) {
                sp = "mapped";
                if (unnamed > 0) {
                    sp += std::to_string(unnamed);
                }
                ++unnamed;
            }
            status = vtkmAddField(vtkmDataSet, m, sp);
            if (!status->continueExecution()) {
                sendText(status->messageType(), status->message());
                return result;
            }
        }
        mapspecies.push_back(sp);
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
        return result;
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

    int idx = 0;
    for (auto &sp: mapspecies) {
        if (auto mapOut = vtkmGetField(isosurface, sp)) {
            mapOut->copyAttributes(mapField[idx]);
            mapOut->setGrid(geoOut);
            updateMeta(mapOut);
            result[idx] = mapOut;
        } else {
            result[idx] = geoOut;
        }
        ++idx;
    }
    result.back() = geoOut;
    return result;
}

bool IsoSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto isoData = task->expect<Vec<Scalar>>(m_dataIn[0]);
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
    std::vector<DataBase::const_ptr> mapFields;
    mapFields.push_back(isoField);

    for (int i = 1; i < NumPorts; ++i) {
        auto mapData = task->accept<Object>(m_dataIn[i]);
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
            mapFields.push_back(mapField);
        }
    }

    if (m_pointOrValue->getValue() == Value) {
        auto objs = work(grid, mapFields, m_isovalue->getValue());
        for (int i = 0; i < NumPorts; ++i) {
            task->addObject(m_dataOut[i], objs[i]);
        }
        task->addObject(m_surfOut, objs.back());
        return true;
    } else {
        std::lock_guard<std::mutex> guard(m_mutex);
        BlockData b(grid, mapFields);
        int t = b.getTimestep();
        m_blocksForTime[t].emplace_back(b);
        return true;
    }

    return true;
}
