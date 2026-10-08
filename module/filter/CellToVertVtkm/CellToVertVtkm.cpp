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

ModuleStatusPtr CellToVertVtkm::prepareInputField(const vistle::Port *port, InputData &input, int index) const
{
    auto field = input.fields[index];
    auto mapping = field->guessMapping(input.vistleGrid);
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
        return VtkmModule::prepareInputField(port, input, index);
    }

    return Success();
}

std::unique_ptr<viskores::filter::Filter> CellToVertVtkm::setUpFilter(const VtkmModule::InputData &input) const
{
#ifdef VERTTOCELL
    auto filter = std::make_unique<viskores::filter::field_conversion::CellAverage>();
#else
    auto filter = std::make_unique<viskores::filter::field_conversion::PointAverage>();
#endif
    return filter;
}

Object::const_ptr CellToVertVtkm::prepareOutputGrid(const InputData &input, OutputData &output) const
{
    return input.vistleGrid;
}

DataBase::ptr CellToVertVtkm::prepareOutputField(const InputData &input, OutputData &output, int index,
                                                 const std::string &fieldName) const
{
    if (output.viskoresDataset.HasField(fieldName)) {
        auto outputField = VtkmModule::prepareOutputField(input, output, index, fieldName);

        // we want the output grid to be the same as the input grid, this way the output fields too will all
        // be mapped to the same grid
#ifdef VERTTOCELL
        outputField->setMapping(DataBase::Element);
#else
        outputField->setMapping(DataBase::Vertex);
#endif
        outputField->setGrid(input.vistleGrid);

        return outputField;
    } else {
        sendInfo("No filter applied for " + fieldName);
        auto ndata = input.fields[index]->clone();
        ndata->setGrid(input.vistleGrid);
        updateMeta(ndata);

        return ndata;
    }
}
