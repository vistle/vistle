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
: VtkmModule2(name, moduleID, comm, 3)
{}

CellToVertVtkm::~CellToVertVtkm()
{}

void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const
{
    if (input.HasField(fieldName)) {
#ifdef VERTTOCELL
        auto filter = vtkm::filter::field_conversion::CellAverage();
#else
        auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
        filter.SetActiveField(fieldName);
        output = filter.Execute(input);
    }
}

ModuleStatusPtr CellToVertVtkm::checkInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                           const std::string &portName) const
{
    auto status = VtkmModule2::checkInputField(grid, field, portName);
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
        return VtkmModule2::transformInputField(grid, field, fieldName, dataset);
    }

    return Success();
}

bool CellToVertVtkm::prepareOutput(const std::shared_ptr<BlockTask> &task, Port *port, vtkm::cont::DataSet &dataset,
                                   Object::ptr &outputGrid, Object::const_ptr &inputGrid,
                                   DataBase::const_ptr &inputField, std::string &fieldName,
                                   DataBase::ptr &outputField) const
{
    if (dataset.HasField(fieldName)) {
        VtkmModule2::prepareOutput(task, port, dataset, outputGrid, inputGrid, inputField, fieldName, outputField);

        if (outputField) {
            outputGrid = prepareOutputGrid(dataset, inputGrid, outputGrid);

#ifdef VERTTOCELL
            outputField->setMapping(DataBase::Element);
#else
            outputField->setMapping(DataBase::Vertex);
#endif
        }
    } else {
        sendInfo("No filter applied for " + port->getName());
        auto ndata = inputField->clone();
        ndata->setGrid(inputGrid);
        updateMeta(ndata);
        task->addObject(port, ndata);
    }
    return true;
}
