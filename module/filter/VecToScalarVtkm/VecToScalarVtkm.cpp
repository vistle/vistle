#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <vistle/vtkm/convert.h>

#include "VecToScalarVtkm.h"

#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>


MODULE_MAIN(VecToScalarVtkm)

using namespace vistle;



VecToScalarVtkm::VecToScalarVtkm(const std::string &name, int moduleID, mpi::communicator comm)
    : VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{
    m_caseParam = addIntParameter("choose_scalar_value", "Choose Scalar Value", 3, Parameter::Choice);
    setParameterChoices(m_caseParam, std::vector<std::string>{"X","Y","Z","Magnitude"});

}

VecToScalarVtkm::~VecToScalarVtkm()
{
}

class ExtractComponentFilter : public viskores::filter::Filter {
public:
  void SetComponentIndex(int c) { comp_ = c; }
private:
  int comp_ = 0;

  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &in) override {
    auto inField = this->GetFieldFromDataSet(in);
    auto assoc   = inField.GetAssociation();
    auto name    = inField.GetName();

    using BaseT = vistle::Scalar; // <- key line
    auto ua = inField.GetData();
    auto compAH = ua.ExtractComponent<BaseT>(comp_);  // <-- now compiles
    viskores::cont::DataSet out = in;
    out.AddField(viskores::cont::Field(name, assoc, compAH));
    return out;
  }
};



std::unique_ptr<viskores::filter::Filter> VecToScalarVtkm::setUpFilter() const {
  const int choice = m_caseParam->getValue(); // 0=X,1=Y,2=Z,3=Mag
  if (choice == 3) {
    auto f = std::make_unique<viskores::filter::vector_analysis::VectorMagnitude>();
    filter_ = f.get();
    return f;
  } else {
    auto f = std::make_unique<ExtractComponentFilter>();
    f->SetComponentIndex(choice);
    filter_ = f.get();
    return f;
  }
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
