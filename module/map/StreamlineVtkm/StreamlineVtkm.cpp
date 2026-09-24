#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/resampling/Probe.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/mpi-cast.h>
#include <viskores/VectorAnalysis.h>

#include <vistle/util/enum.h>
#include <vistle/vtkm/convert.h>
#include <vistle/vtkm/vtkm_module_utils.h>

#include "worklet/GenerateSeeds.h"
#include "StreamlineVtkm.h"

using namespace vistle;

MODULE_MAIN(StreamlineVtkm)

DEFINE_ENUM_WITH_STRING_CONVERSIONS(IntegrationMethod, (Euler)(RK4))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(StartStyle, (Line)(Plane))

void StreamlineVtkm::GlobalData::clear()
{
    partitionedDatasets.clear();
    inputGrids.clear();
    inputFields.clear();
}

bool StreamlineVtkm::GlobalData::isEmpty() const
{
    return partitionedDatasets.empty() && inputGrids.empty() && inputFields.empty();
}

bool StreamlineVtkm::GlobalData::getDataAtIndex(unsigned int index, std::vector<vistle::Object::const_ptr> &grids,
                                                std::vector<std::vector<vistle::DataBase::const_ptr>> &fieldsPerPort,
                                                viskores::cont::PartitionedDataSet &dataset)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (index >= this->partitionedDatasets.size())
        return false;

    dataset = this->partitionedDatasets[index];
    if (dataset.GetNumberOfPartitions() == 0)
        return false;

    if (index < this->inputGrids.size())
        grids = this->inputGrids[index];

    fieldsPerPort.resize(this->inputFields.size());
    for (unsigned int port = 0; port < this->inputFields.size(); port++) {
        if (index < this->inputFields[port].size())
            fieldsPerPort[port] = this->inputFields[port][index];
    }

    return true;
}

void StreamlineVtkm::GlobalData::resize(std::size_t newSize, std::size_t numFields)
{
    if (inputFields.size() < numFields)
        inputFields.resize(numFields);

    if (partitionedDatasets.size() < newSize) {
        partitionedDatasets.resize(newSize);
        inputGrids.resize(newSize);
        for (auto &field: inputFields)
            field.resize(newSize);
    }
}

StreamlineVtkm::StreamlineVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createModulePorts();
    createModuleParameters();

    setReducePolicy(message::ReducePolicy::PerTimestep);
    setViskoresMPICommunicatorToCopyOf(comm);
}

bool StreamlineVtkm::prepare()
{
    if (!m_inputPorts[0]->isConnected()) {
        if (rank() == 0)
            sendError("No input connected to %s", m_inputPorts[0]->getName().c_str());
        return false;
    }

    for (int i = 0; i < m_numPorts; ++i) {
        if (!m_inputPorts[i]->isConnected() && m_outputPorts[i]->isConnected()) {
            if (rank() == 0)
                sendError("Output port " + m_outputPorts[i]->getName() +
                          " is connected, but corresponding input port " + m_inputPorts[i]->getName() + " is not");
            return false;
        }
    }

    std::lock_guard<std::mutex> lock(m_globalData.mutex);
    if (!m_globalData.isEmpty())
        m_globalData.clear();

    return Module::prepare();
}

bool StreamlineVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    vistle::Object::const_ptr vistleGrid;
    std::vector<vistle::DataBase::const_ptr> fields;
    auto status = readInAndVerifyPorts(task, vistleGrid, fields);
    if (!checkAndNotify(status))
        return true;

    assert(m_outputPorts.size() == fields.size());

    auto timestep = vistleGrid->getTimestep();
    if (fields[0] && (timestep != fields[0]->getTimestep())) {
        sendError("timestep mismatch: grid = %d, field = %d", timestep, fields[0]->getTimestep());
        return true;
    }
    if (timestep < 0)
        timestep = -1;

    viskores::cont::DataSet viskoresDataset;
    status = transformInputToViskores(vistleGrid, fields, viskoresDataset);
    if (!checkAndNotify(status))
        return true;

    std::lock_guard<std::mutex> lock(m_globalData.mutex);
    auto numSteps = static_cast<std::size_t>(timestep + 1);
    m_globalData.resize(numSteps + 1, fields.size());

    m_globalData.partitionedDatasets[numSteps].AppendPartitions({viskoresDataset});
    m_globalData.inputGrids[numSteps].push_back(vistleGrid);
    for (std::size_t i = 0; i < fields.size(); ++i)
        m_globalData.inputFields[i][numSteps].push_back(fields[i]);

    return true;
}

bool StreamlineVtkm::reduce(int timestep)
{
    std::vector<vistle::Object::const_ptr> inputGrids;
    std::vector<std::vector<vistle::DataBase::const_ptr>> inputFields;
    viskores::cont::PartitionedDataSet inputPartitionedDataset;
    if (!m_globalData.getDataAtIndex(static_cast<unsigned int>(timestep + 1), inputGrids, inputFields,
                                     inputPartitionedDataset))
        return true;

    viskores::cont::PartitionedDataSet output;
    auto streamlineFilter = setUpFilter();
    if (!this->tryToExecuteFilter(*streamlineFilter, inputPartitionedDataset, output))
        return true;

    // the Streamline filter can drop input partitions that never received a particle and does not
    // otherwise report which input partition an output partition came from, so the output partitions
    // cannot be matched to the input one. For this reason, we have to merge all blocks of this timestep
    // into a single dataset to probe fields from.
    auto mergedInputDataset = viskores::cont::MergePartitionedDataSet(inputPartitionedDataset);

    for (viskores::Id partitionIndex = 0; partitionIndex < output.GetNumberOfPartitions(); partitionIndex++) {
        const auto &dataset = output.GetPartition(partitionIndex);
        auto outputGrid = vtkmGetGeometry(dataset);
        if (!outputGrid) {
            sendError("Could not convert StreamlineVtkm output geometry to a Vistle object.");
            continue;
        }

        outputGrid->setMeta(getMetaForOutput(timestep));
        updateMeta(outputGrid);
        // attributes (e.g. species name, color map range) are the same on every block, so let's just
        // use the first one (since we can't match the input to the output partitions)
        if (!inputGrids.empty() && inputGrids.front())
            outputGrid->copyAttributes(inputGrids.front());

        for (int port = 0; port < m_numPorts; port++) {
            if (!m_outputPorts[port]->isConnected())
                continue;

            // the Streamline filter does not map any input fields onto the generated streamlines,
            // so we have to probe them from the (merged) input dataset with the Probe filter
            auto probeFilter = viskores::filter::resampling::Probe();
            probeFilter.SetGeometry(dataset);
            probeFilter.SetOutputFieldName(getFieldName(port));

            DataBase::ptr field;
            viskores::cont::DataSet probeOutput;
            if (this->tryToExecuteFilter(probeFilter, mergedInputDataset, probeOutput))
                field = vtkmGetField(probeOutput, getFieldName(port));

            if (field) {
                field->setMeta(getMetaForOutput(timestep));
                updateMeta(field);
                // again, we simply take the first field to copy the attributes from, since we can't match the
                // input to the output partitions
                if (static_cast<std::size_t>(port) < inputFields.size() && !inputFields[port].empty()) {
                    if (auto inputField = inputFields[port].front()) {
                        auto mapping = field->mapping();
                        field->copyAttributes(inputField);
                        field->setMapping(mapping);
                    }
                }

                field->setGrid(outputGrid);
                addObject(m_outputPorts[port], field);
            } else {
                addObject(m_outputPorts[port], outputGrid);
            }
        }
    }

    return true;
}

bool StreamlineVtkm::changeParameter(const Parameter *param)
{
    if (param == m_startStyle) {
        setParameterReadOnly(m_direction, m_startStyle->getValue() != Plane);
    } else if (param == m_maxNumberOfSeeds) {
        setParameterRange(m_numberOfSeeds, (Integer)1, m_maxNumberOfSeeds->getValue());
    }

    return Module::changeParameter(param);
}

ModuleStatusPtr StreamlineVtkm::readInAndVerifyPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                                     std::vector<DataBase::const_ptr> &fields) const
{
    for (int i = 0; i < m_numPorts; ++i) {
        if (!m_inputPorts[i]->isConnected()) {
            fields.push_back(nullptr);
            continue;
        }

        auto container = task->accept<Object>(m_inputPorts[i]);
        auto split = splitContainerObject(container);
        auto geometry = split.geometry;
        auto data = split.mapped;

        // make sure there is data on the input port if the corresponding output port is connected
        if (!geometry && !data)
            return Error("No data on input port " + m_inputPorts[i]->getName() + ", even though it is connected");

        // Streamlines filter expects data to be three-dimensional
        if (i == 0 && data && !Vec<Scalar, 3>::as(data))
            return Error("Input field at port " + m_inputPorts[i]->getName() + " must be a 3D vector field!");

        fields.push_back(data);

        // make sure all data fields are defined on the same grid
        if (grid) {
            if (geometry && geometry->getHandle() != grid->getHandle()) {
                return Error("The grid on " + m_inputPorts[i]->getName() +
                             " does not match the grid on the other input ports!");
            }
        } else {
            grid = geometry;
        }
    }

    if (!grid)
        return Error("Could not find a valid input grid on any input port");

    return Success();
}

ModuleStatusPtr StreamlineVtkm::transformInputToViskores(const Object::const_ptr &grid,
                                                         const std::vector<DataBase::const_ptr> &fields,
                                                         viskores::cont::DataSet &dataset) const
{
    auto status = vtkmSetGrid(dataset, grid);
    if (!status->continueExecution())
        return status;

    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (fields[i]) {
            status = vtkmAddField(dataset, fields[i], getFieldName(i));
            if (!status->continueExecution())
                return status;
        }
    }

    return Success();
}

namespace {
template<typename S>
viskores::Vec3f make_Vec3f(const vistle::ParameterVector<S> &v)
{
    typedef viskores::FloatDefault F;
    return viskores::Vec3f{static_cast<F>(v[0]), static_cast<F>(v[1]), static_cast<F>(v[2])};
}
} // namespace

viskores::cont::ArrayHandle<viskores::Particle> StreamlineVtkm::createSeedArray() const
{
    /*
        Since in Vistle VectorParameters (i.e., vector module parameters) are always vectors of
        doubles (independent of what the user sets VISTLE_DOUBLE_PRECISION to), and Viskores (in
        particular the array of viskores::Particles that the Streamline filter needs) strictly
        uses viskores::FloatDefault (which is dependent of VISTLE_DOUBLE_PRECISION), we have to
        cast the start points and direction to viskores::FloatDefault here.
    */
    viskores::Vec3f startpoint1 = make_Vec3f(m_startPoint1->getValue());
    viskores::Vec3f startpoint2 = make_Vec3f(m_startPoint2->getValue());

    if (m_startStyle->getValue() == StartStyle::Line) {
        return generateSeedsOnLine(m_numberOfSeeds->getValue(), startpoint1, startpoint2);

    } else {
        viskores::Vec3f direction = make_Vec3f(m_direction->getValue());
        return generateSeedsOnPlane(m_numberOfSeeds->getValue(), startpoint1, startpoint2, direction);
    }
}

std::unique_ptr<viskores::filter::Filter> StreamlineVtkm::setUpFilter() const
{
    auto filter = std::make_unique<viskores::filter::flow::Streamline>();

    filter->SetStepSize(m_stepSize->getValue());
    filter->SetNumberOfSteps(m_numberOfSteps->getValue());

    auto seedArray = createSeedArray();
    filter->SetSeeds(seedArray);

    if (m_integrationMethod->getValue() == IntegrationMethod::Euler)
        filter->SetSolverEuler();
    else
        filter->SetSolverRK4();

    filter->SetActiveField(getFieldName(0));
    filter->SetOutputFieldName(getFieldName(0, true));
    filter->SetFieldsToPass("", viskores::cont::Field::Association::Any, viskores::filter::FieldSelection::Mode::All);

    return filter;
}

bool StreamlineVtkm::tryToExecuteFilter(viskores::filter::Filter &filter, const viskores::cont::DataSet &inputDataset,
                                        viskores::cont::DataSet &outputDataset) const
{
    return vistle::vtkm::tryToExecuteFilter(*this, filter, inputDataset, outputDataset);
}

bool StreamlineVtkm::tryToExecuteFilter(viskores::filter::Filter &filter,
                                        const viskores::cont::PartitionedDataSet &inputDataset,
                                        viskores::cont::PartitionedDataSet &outputDataset) const
{
    return vistle::vtkm::tryToExecuteFilter(*this, filter, inputDataset, outputDataset);
}

bool StreamlineVtkm::checkAndNotify(const ModuleStatusPtr &status) const
{
    return vistle::vtkm::checkAndNotify(*this, status);
}

void StreamlineVtkm::createModulePorts()
{
    for (int i = 0; i < m_numPorts; ++i) {
        m_inputPorts.push_back(createInputPort(getPortName(i, false), "input grid with mapped data"));
        m_outputPorts.push_back(createOutputPort(getPortName(i, true), "output grid with mapped data"));

        linkPorts(m_inputPorts[i], m_outputPorts[i]);

        if (i > 0)
            setPortOptional(m_inputPorts[i], true);
    }
}

void StreamlineVtkm::createModuleParameters()
{
    setCurrentParameterGroup("Seed Points");
    const Integer max_no_seeds = 300;
    m_numberOfSeeds = addIntParameter("no_startp", "number of seed points", 2);
    setParameterRange(m_numberOfSeeds, (Integer)1, max_no_seeds);

    m_startStyle =
        addIntParameter("startStyle", "initial particle position configuration", StartStyle::Line, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_startStyle, StartStyle, );

    m_startPoint1 = addVectorParameter("startpoint1", "1st initial point", ParamVector(0, 0.2, 0));
    m_startPoint2 = addVectorParameter("startpoint2", "2nd initial point", ParamVector(1, 0, 0));

    m_direction = addVectorParameter("direction", "tracing direction", ParamVector(1, 0, 0));

    m_maxNumberOfSeeds =
        addIntParameter("max_no_startp", "maximum number of seeds (for parameter/slider limits)", max_no_seeds);
    setParameterRange(m_maxNumberOfSeeds, (Integer)2, (Integer)1000000);

    setCurrentParameterGroup("Stop Conditions");
    m_numberOfSteps = addIntParameter("steps_max", "maximum number of integration steps", 1000);

    setCurrentParameterGroup("Step Length Control");
    m_integrationMethod =
        addIntParameter("integration", "integration method", IntegrationMethod::RK4, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_integrationMethod, IntegrationMethod, );

    m_stepSize = addFloatParameter("step_size", "integration step size", 0.1f);
}

void StreamlineVtkm::setViskoresMPICommunicatorToCopyOf(const mpi::communicator &comm) const
{
    MPI_Comm dupComm;
    MPI_Comm_dup(MPI_Comm(comm), &dupComm);
    viskoresdiy::mpi::communicator viskoresComm(viskoresdiy::mpi::make_DIY_MPI_Comm(dupComm), false);

    viskores::cont::EnvironmentTracker::SetCommunicator(viskoresComm);
}

std::string StreamlineVtkm::getFieldName(int index, bool output) const
{
    std::string name = "data_at_port_" + std::to_string(index);
    if (index == 0 && output)
        name += "_out";
    return name;
}

vistle::Meta StreamlineVtkm::getMetaForOutput(int timestep) const
{
    Meta meta;
    meta.setNumBlocks(size());
    meta.setBlock(rank());
    meta.setNumTimesteps(numTimesteps() > 0 ? numTimesteps() : -1);
    meta.setTimeStep(numTimesteps() > 0 ? timestep : -1);

    return meta;
}

std::string StreamlineVtkm::getPortName(int index, bool output) const
{
    std::string portName = output ? "data_out" : "data_in";
    portName += std::to_string(index);

    return portName;
}
