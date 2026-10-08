#include "vtkm_module.h"

#include <boost/fusion/sequence/io/out.hpp>
#include <sstream>

#include "convert.h"
#include "vtkm_module_utils.h"

using namespace vistle;


VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, int numPorts,
                       MappedDataHandling mode)
: Module(name, moduleID, comm), m_numPorts(numPorts), m_mappedDataHandling(mode)
{
    assert(m_numPorts > 0);
    bool dataInput =
        m_mappedDataHandling != MappedDataHandling::Discard && m_mappedDataHandling != MappedDataHandling::Generate;
    bool dataOutput = m_mappedDataHandling != MappedDataHandling::Discard;

    for (int i = 0; i < m_numPorts; ++i) {
        std::string in("data_in");
        std::string out("data_out");
        if (i > 0) {
            in += std::to_string(i);
            out += std::to_string(i);
        }
        m_inputPorts.push_back(createInputPort(in, dataInput ? "input grid with mapped data" : "input grid"));
        m_outputPorts.push_back(createOutputPort(out, dataOutput ? "output grid with mapped data" : "output grid"));
        linkPorts(m_inputPorts[i], m_outputPorts[i]);
        if (i > 0) {
            setPortOptional(m_inputPorts[i], true);
        }
    }

    m_printObjectInfo =
        addIntParameter("_print_object_info", "print information on generated data objects for debug purposes", false,
                        Parameter::Boolean);
}

std::string VtkmModule::getFieldName(int index, bool output) const
{
    std::string name = "data_at_port_" + std::to_string(index);
    if (index == 0 && output)
        name += "_out";
    return name;
}

bool VtkmModule::prepare()
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

    return Module::prepare();
}

ModuleStatusPtr VtkmModule::readInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
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

ModuleStatusPtr VtkmModule::prepareInputGrid(InputData &input) const
{
    return vtkmSetGrid(input.viskoresDataset, input.vistleGrid);
}

ModuleStatusPtr VtkmModule::prepareInputField(const Port *port, InputData &input, int index) const
{
    return vtkmAddField(input.viskoresDataset, input.fields[index], getFieldName(index));
}

Object::const_ptr VtkmModule::prepareOutputGrid(const InputData &input, OutputData &output) const
{
    auto outputGrid = vtkmGetGeometry(output.viskoresDataset);
    if (outputGrid) {
        updateMeta(outputGrid);
        outputGrid->copyAttributes(input.vistleGrid);
    }

    return outputGrid;
}

DataBase::ptr VtkmModule::prepareOutputField(const InputData &input, OutputData &output, int index,
                                             const std::string &fieldName) const
{
    if (auto mapped = vtkmGetField(output.viskoresDataset, fieldName)) {
        std::cerr << "mapped data: " << *mapped << std::endl;
        updateMeta(mapped);

        // the mapping of the output field might differ from the one of the input field,
        // so lets temporarily store it and set it again after copying the attributes
        auto mapping = mapped->mapping();
        mapped->copyAttributes(input.fields[index]);
        mapped->setMapping(mapping);

        if (output.vistleGrid)
            mapped->setGrid(output.vistleGrid);

        return mapped;

    } else {
        sendError("An error occurred while transforming the filter output field " + fieldName + " to a Vistle object.");
    }

    return nullptr;
}

bool VtkmModule::compute(const std::shared_ptr<BlockTask> &task) const
{
    InputData input;
    OutputData output;

    bool printInfo = m_printObjectInfo->getValue() != 0;

    // read in data from the input ports...
    auto status = readInPorts(task, input.vistleGrid, input.fields);
    if (!checkAndNotify(status))
        return true;

    assert(m_outputPorts.size() == input.fields.size());

    // ... transform the input grid (and fields) into a Viskores dataset ...
    status = prepareInputGrid(input);
    if (!checkAndNotify(status))
        return true;

    for (std::size_t i = 0; i < input.fields.size(); ++i) {
        if (i > 0 && !m_outputPorts[i]->isConnected())
            continue;

        if (m_mappedDataHandling == MappedDataHandling::Require) {
            if (!input.fields[i]) {
                sendError("Cannot continue: No mapped data on input port " + m_inputPorts[i]->getName() +
                          ", even though it is required by the module!");
                return true;
            }
        }
        if (input.fields[i]) {
            status = prepareInputField(m_inputPorts[i], input, i);
            if (!checkAndNotify(status))
                return true;
        }
    }

    // ... run filter on the active field ...
    bool useInputData =
        m_mappedDataHandling != MappedDataHandling::Discard && m_mappedDataHandling != MappedDataHandling::Generate;
    auto activeField = useInputData ? getFieldName(0) : "";

    if (printInfo) {
        std::stringstream str;
        str << "<pre>Input ";
        input.viskoresDataset.PrintSummary(str);
        str << "</pre>" << std::endl;
        std::cout << str.str() << std::endl;
    }

    if (m_mappedDataHandling != MappedDataHandling::Require || input.viskoresDataset.HasField(activeField)) {
        if (auto filter = setUpFilter(input)) {
            if (input.viskoresDataset.HasField(activeField))
                filter->SetActiveField(activeField);

            /*
                By default, Viskores names output fields the same as input fields which causes problems
                if the input mapping is different from the output mapping, i.e., when converting
                a point field to a cell field or vice versa. To avoid having a point and a
                cell field of the same name in the resulting dataset, which leads to conflicts, e.g.,
                when calling Viskores's GetField() method, we rename the output field here.
            */
            filter->SetOutputFieldName(getFieldName(0, true));
            filter->SetFieldsToPass("", viskores::cont::Field::Association::Any,
                                    viskores::filter::FieldSelection::Mode::All);

            if (printInfo) {
                std::stringstream str;
                str << "Filter: " << typeid(decltype(*filter)).name() << std::endl;
                std::cout << str.str() << std::endl;
            }

            if (!this->tryToExecuteFilter(*filter, input.viskoresDataset, output.viskoresDataset))
                return true;

            if (printInfo) {
                std::stringstream str;
                str << "<pre>Output ";
                output.viskoresDataset.PrintSummary(str);
                str << "</pre>" << std::endl;
                std::cout << str.str() << std::endl;
            }
        } else {
            output.viskoresDataset = input.viskoresDataset;

            if (printInfo) {
                std::stringstream str;
                str << "<pre>Output ";
                output.viskoresDataset.PrintSummary(str);
                str << "</pre>" << std::endl;
                std::cout << str.str() << std::endl;
            }
        }
    }

    // ... transform filter output, i.e., grid and data fields, to Vistle objects ...
    output.vistleGrid = prepareOutputGrid(input, output);
    if (!output.vistleGrid)
        return true;

    output.fields.resize(input.fields.size(), nullptr);
    for (std::size_t i = 0; i < input.fields.size(); ++i) {
        if (!m_outputPorts[i]->isConnected())
            continue;

        if (m_mappedDataHandling != MappedDataHandling::Use || input.fields[i]) {
            std::string outputFieldName = getFieldName(i);
            if (i == 0 && output.viskoresDataset.HasField(getFieldName(i, true))) {
                // if filter has created a dedicated output field, use it
                outputFieldName = getFieldName(i, true);
            }
            output.fields[i] = prepareOutputField(input, output, i, outputFieldName);
        }

        // ... and write the result to the output ports
        if (output.fields[i] || m_mappedDataHandling == MappedDataHandling::Generate) {
            task->addObject(m_outputPorts[i], output.fields[i]);
        } else if (output.vistleGrid) {
            task->addObject(m_outputPorts[i], output.vistleGrid);
        }
    }

    return true;
}

bool VtkmModule::checkAndNotify(const ModuleStatusPtr &status) const
{
    return vistle::vtkm::checkAndNotify(*this, status);
}

bool VtkmModule::tryToExecuteFilter(viskores::filter::Filter &filter, const viskores::cont::DataSet &inputDataset,
                                    viskores::cont::DataSet &outputDataset) const
{
    return vistle::vtkm::tryToExecuteFilter(*this, filter, inputDataset, outputDataset);
}
