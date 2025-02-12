
#include <vistle/alg/objalg.h>

#include "convert.h"
#include "convert_status.h"

#include "VtkmModule.h"

using namespace vistle;

VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_mapDataIn = createInputPort("data_in", "input data");
    m_dataOut = createOutputPort("data_out", "output data");
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
    vtkm::cont::DataSet inputData;
    vtkm::cont::DataSet outputData;


    Object::const_ptr grid;
    DataBase::const_ptr field;

    auto status = inputToVistle(task, inputData, grid, field);
    if (!checkStatus(status))
        return true;


    runFilter(inputData, outputData);

    outputToVtkm(task, outputData, grid, field);

    return true;
}

std::unique_ptr<ConvertStatus> VtkmModule::inputToVistle(const std::shared_ptr<vistle::BlockTask> &task,
                                                         vtkm::cont::DataSet &inputData,
                                                         vistle::Object::const_ptr &grid,
                                                         vistle::DataBase::const_ptr &field) const
{ // check input grid and input field
    auto container = task->accept<Object>(m_mapDataIn);
    auto split = splitContainerObject(container);
    field = split.mapped;
    if (!field)
        return Error("No mapped data on input grid!");

    grid = split.geometry;
    if (!grid)
        return Error("Input grid is missing!");

    // transform Vistle grid to VTK-m dataset
    auto status = vtkmSetGrid(inputData, grid);
    if (!checkStatus(status))
        return status;

    if (field) {
        m_mapSpecies = field->getAttribute("_species");
        if (m_mapSpecies.empty())
            m_mapSpecies = "mapdata";
        status = vtkmAddField(inputData, field, m_mapSpecies);
        if (!checkStatus(status))
            return status;
    }

    return Success();
}

bool VtkmModule::outputToVtkm(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &outputData,
                              vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field) const
{
    // transform result back to Vistle
    Object::ptr geoOut = vtkmGetGeometry(outputData);

    // add result to output
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(grid);
        geoOut->setTransform(grid->getTransform());
        if (geoOut->getTimestep() < 0) {
            geoOut->setTimestep(grid->getTimestep());
            geoOut->setNumTimesteps(grid->getNumTimesteps());
        }
        if (geoOut->getBlock() < 0) {
            geoOut->setBlock(grid->getBlock());
            geoOut->setNumBlocks(grid->getNumBlocks());
        }
    }

    if (field) {
        if (auto mapped = vtkmGetField(outputData, m_mapSpecies)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            mapped->copyAttributes(field);
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
