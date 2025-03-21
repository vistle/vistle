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
: VtkmModule(name, moduleID, comm)
{}

CellToVertVtkm::~CellToVertVtkm()
{}

ModuleStatusPtr CellToVertVtkm::transformInputField(const Port *port, const Object::const_ptr &grid,
                                                    const DataBase::const_ptr &field, std::string &fieldName,
                                                    vtkm::cont::DataSet &dataset) const
{
    auto mapping = field->guessMapping(grid);
    // ... make sure the mapping is either vertex or element
    if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
        std::stringstream msg;
        msg << "Unsupported data mapping " << field->mapping() << ", guessed=" << mapping << " on " << field->getName()
            << " at port " << port->getName();
        return Error(msg.str());
    }

    // ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        // transform to VTK-m + add to dataset
        return VtkmModule::transformInputField(port, grid, field, fieldName, dataset);
    }

    return Success();
}

std::unique_ptr<vtkm::filter::Filter> CellToVertVtkm::setUpFilter() const
{
#ifdef VERTTOCELL
    auto filter = std::make_unique<vtkm::filter::field_conversion::CellAverage>();
#else
    auto filter = std::make_unique<vtkm::filter::field_conversion::PointAverage>();
#endif
    return filter;
}

Object::ptr CellToVertVtkm::prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                              const Object::const_ptr &inputGrid) const
{
    return nullptr;
}

DataBase::ptr CellToVertVtkm::prepareOutputField(const vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid,
                                                 const DataBase::const_ptr &inputField, const std::string &fieldName,
                                                 const Object::ptr &outputGrid) const
{
    if (dataset.HasField(fieldName)) {
        auto outputField = VtkmModule::prepareOutputField(dataset, inputGrid, inputField, fieldName, outputGrid);

        // we want the output grid to be the same as the input grid, this way the output fields too will all
        // be mapped to the same grid
#ifdef VERTTOCELL
        outputField->setMapping(DataBase::Element);
#else
        outputField->setMapping(DataBase::Vertex);
#endif
        outputField->setGrid(inputGrid);
        return outputField;
    } else {
        sendInfo("No filter applied for " + fieldName);
        auto ndata = inputField->clone();
        ndata->setGrid(inputGrid);
        updateMeta(ndata);
        return ndata;
    }
}
