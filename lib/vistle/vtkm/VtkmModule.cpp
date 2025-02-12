
#include <vistle/alg/objalg.h>

#include "convert.h"
#include "convert_status.h"

#include "VtkmModule.h"

using namespace vistle;

VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, bool requireMappedData)
: Module(name, moduleID, comm), m_requireMappedData(requireMappedData)
{
    m_dataIn = createInputPort("data_in", m_requireMappedData ? "input grid with mapped data" : "input grid");
    m_dataOut = createOutputPort("data_out", m_requireMappedData ? "output grid with mapped data" : "output grid");
}

VtkmModule::~VtkmModule()
{}

bool VtkmModule::checkStatus(const std::unique_ptr<ConvertStatus> &status) const
{
    if (!status->isSuccessful()) {
        sendError(status->errorMessage());
        return false;
    }
    return true;
}

bool VtkmModule::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    vtkm::cont::DataSet filterInputData;
    vtkm::cont::DataSet filterOutputData;

    Object::const_ptr inputGrid;
    DataBase::const_ptr inputField;

    auto status = inputToVtkm(task, inputGrid, inputField, filterInputData);
    if (!checkStatus(status))
        return true;

    runFilter(filterInputData, filterOutputData);

    outputToVistle(task, filterOutputData, inputGrid, inputField);

    return true;
}

std::unique_ptr<ConvertStatus> VtkmModule::inputToVtkm(const std::shared_ptr<vistle::BlockTask> &task,
                                                       vistle::Object::const_ptr &inputGrid,
                                                       vistle::DataBase::const_ptr &inputField,
                                                       vtkm::cont::DataSet &filterInputData) const
{ // check input grid and input field
    auto container = task->accept<Object>(m_dataIn);
    auto split = splitContainerObject(container);
    inputField = split.mapped;
    if (m_requireMappedData && !inputField)
        return Error("No mapped data on input grid!");

    inputGrid = split.geometry;
    if (!inputGrid)
        return Error("Input grid is missing!");

    // transform Vistle grid to VTK-m dataset
    auto status = vtkmSetGrid(filterInputData, inputGrid);
    if (!checkStatus(status))
        return status;

    if (m_requireMappedData) {
        m_mappedDataName = inputField->getAttribute("_species");
        if (m_mappedDataName.empty())
            m_mappedDataName = "mapdata";
        status = vtkmAddField(filterInputData, inputField, m_mappedDataName);
        if (!checkStatus(status))
            return status;
    }

    return Success();
}

bool VtkmModule::outputToVistle(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &filterOutputData,
                                vistle::Object::const_ptr &inputGrid, vistle::DataBase::const_ptr &inputField) const
{
    // transform result back to Vistle
    Object::ptr geoOut = vtkmGetGeometry(filterOutputData);

    // add result to output
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(inputGrid);
        geoOut->setTransform(inputGrid->getTransform());
        if (geoOut->getTimestep() < 0) {
            geoOut->setTimestep(inputGrid->getTimestep());
            geoOut->setNumTimesteps(inputGrid->getNumTimesteps());
        }
        if (geoOut->getBlock() < 0) {
            geoOut->setBlock(inputGrid->getBlock());
            geoOut->setNumBlocks(inputGrid->getNumBlocks());
        }
    }

    if (m_requireMappedData) {
        if (auto mapped = vtkmGetField(filterOutputData, m_mappedDataName)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            mapped->copyAttributes(inputField);
            mapped->setGrid(geoOut);
            updateMeta(mapped);
            task->addObject(m_dataOut, mapped);
            return true;
        } else {
            sendError("could not handle mapped data");
            task->addObject(m_dataOut, geoOut);
        }
    } else {
        task->addObject(m_dataOut, geoOut);
    }
    return true;
}
