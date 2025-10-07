#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <vistle/vtkm/convert.h>

#include "VecToScalarVtkm.h"

MODULE_MAIN(VecToScalarVtkm)

using namespace vistle;



VecToScalarVtkm::VecToScalarVtkm(const std::string &name, int moduleID, mpi::communicator comm)
    : VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{
}

VecToScalarVtkm::~VecToScalarVtkm()
{
}

std::unique_ptr<viskores::filter::Filter> VecToScalarVtkm::setUpFilter() const
{

    return std::make_unique<viskores::filter::vector_analysis::VectorMagnitude>();

}


ModuleStatusPtr VecToScalarVtkm::prepareInputField(const Port *port, const Object::const_ptr &grid,
                                                  const DataBase::const_ptr &field, std::string &fieldName,
                                                  viskores::cont::DataSet &dataset) const
{
    
        // transform to Viskores + add to dataset
        return VtkmModule::prepareInputField(port, grid, field, fieldName, dataset);
}



Object::const_ptr VecToScalarVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                    const Object::const_ptr &inputGrid) const
{
    return nullptr;
}



DataBase::ptr VecToScalarVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                    const Object::const_ptr &inputGrid,
                                    const DataBase::const_ptr &inputField,
                                    const std::string &fieldName,
                                    const Object::const_ptr &outputGrid) const
{
    auto out = VtkmModule::prepareOutputField(dataset, inputGrid, inputField, fieldName, outputGrid);
    if (!out)
        return nullptr;

    // preserve association (Vertex/Element) and attach a valid grid
    out->setMapping(inputField->mapping());
    out->setGrid(inputGrid); // important because outputGrid == nullptr
    return out;
}
