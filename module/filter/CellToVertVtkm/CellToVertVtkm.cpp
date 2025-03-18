#include <sstream>

#ifdef VERTTOCELL
#include <vtkm/filter/field_conversion/CellAverage.h>
#else
#include <vtkm/filter/field_conversion/PointAverage.h>
#endif

#include <vistle/vtkm/convert.h>

#include "CellToVertVtkm.h"

MODULE_MAIN(CellToVertVtkm)

using namespace vistle;

CellToVertVtkm::CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1)
{}

CellToVertVtkm::~CellToVertVtkm()
{}

ModuleStatusPtr CellToVertVtkm::checkInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                                const std::string &portName) const
{
    auto status = VtkmModule::checkInputField(grid, field, portName);
    if (!isValid(status)) {
        return status;
    }

    auto mapping = field->guessMapping(grid);
    // ... make sure the mapping is either vertex or element
    if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
        std::stringstream msg;
        msg << "Unsupported data mapping " << field->mapping() << ", guessed=" << mapping << " on " << field->getName();
        return Error(msg.str().c_str());
    }
    return Success();
}

ModuleStatusPtr CellToVertVtkm::transformInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                                    std::string &fieldName, vtkm::cont::DataSet &dataset) const
{
    auto mapping = field->guessMapping(grid);
    // ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        // transform to VTK-m + add to dataset
        return VtkmModule::transformInputField(grid, field, fieldName, dataset);
    }

    return Success();
}

void CellToVertVtkm::runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                               vtkm::cont::DataSet &output) const
{
    if (input.HasField(fieldName)) {
#ifdef VERTTOCELL
        auto filter = vtkm::filter::field_conversion::CellAverage();
#else
        auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
        filter.SetActiveField(fieldName);
        /*
            By default, VTK-m names output fields the same as input fields which causes problems
            if the input mapping is different from the output mapping, i.e., when converting 
            a point field to a cell field or vice versa. To avoid having a point and a 
            cell field of the same name in the resulting dataset, which leads to conflicts, e.g., 
            when calling VTK-m's GetField() method, we rename the output field here.
        */
        filter.SetOutputFieldName("output_" + fieldName);
        output = filter.Execute(input);
    }
}

Object::ptr CellToVertVtkm::prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                              const Object::const_ptr &inputGrid) const
{
    return inputGrid->clone();
}

DataBase::ptr CellToVertVtkm::prepareOutputField(const vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid,
                                                 const DataBase::const_ptr &inputField, const std::string &fieldName,
                                                 const Object::ptr &outputGrid) const
{
    std::string outputFieldName = "output_" + fieldName;
    if (dataset.HasField(outputFieldName)) {
        auto outputField = VtkmModule::prepareOutputField(dataset, inputGrid, inputField, outputFieldName, outputGrid);
#ifdef VERTTOCELL
        outputField->setMapping(DataBase::Element);
#else
        outputField->setMapping(DataBase::Vertex);
#endif
        return outputField;
    }
    return nullptr;
}

void CellToVertVtkm::writeResultToPort(const std::shared_ptr<BlockTask> &task, const Object::const_ptr &inputGrid,
                                       const DataBase::const_ptr &inputField, Port *port, Object::ptr &outputGrid,
                                       DataBase::ptr &outputField) const
{
    if (outputField) {
        task->addObject(port, outputField);
    } else {
        sendInfo("No filter applied for " + port->getName());
        auto ndata = inputField->clone();
        ndata->setGrid(inputGrid);
        updateMeta(ndata);
        task->addObject(port, ndata);
    }
}
