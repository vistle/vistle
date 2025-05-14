#include <sstream>

#ifdef VERTTOCELL
#include <viskores/filter/field_conversion/CellAverage.h>
#else
#include <viskores/filter/field_conversion/PointAverage.h>
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

ModuleStatusPtr CellToVertVtkm::prepareInputField(const Port *port, const Object::const_ptr &grid,
                                                  const DataBase::const_ptr &field, std::string &fieldName,
                                                  viskores::cont::DataSet &dataset) const
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
        // transform to Viskores + add to dataset
        return VtkmModule::prepareInputField(port, grid, field, fieldName, dataset);
    }

    return Success();
}

std::unique_ptr<viskores::filter::Filter> CellToVertVtkm::setUpFilter() const
{
#ifdef VERTTOCELL
    auto filter = std::make_unique<viskores::filter::field_conversion::CellAverage>();
#else
    auto filter = std::make_unique<viskores::filter::field_conversion::PointAverage>();
#endif
    return filter;
}

Object::const_ptr CellToVertVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                    const Object::const_ptr &inputGrid) const
{
    return nullptr;
}

DataBase::ptr CellToVertVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                                 const Object::const_ptr &inputGrid,
                                                 const DataBase::const_ptr &inputField, const std::string &fieldName,
                                                 const Object::const_ptr &outputGrid) const
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
