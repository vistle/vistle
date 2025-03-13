#include <vtkm/filter/contour/Slice.h>

#include "CuttingSurfaceVtkm.h"

MODULE_MAIN(CuttingSurfaceVtkm)

using namespace vistle;

CuttingSurfaceVtkm::CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm), m_implFuncControl(this)
{
    m_implFuncControl.init();
    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 0, Parameter::Boolean);

    // this makes sure module is executed as soon as pick interactor pose is changed in the GUI
    setCacheMode(ObjectCache::CacheDeleteEarly);
}

CuttingSurfaceVtkm::~CuttingSurfaceVtkm()
{}

bool CuttingSurfaceVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}

void CuttingSurfaceVtkm::runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                                   vtkm::cont::DataSet &output) const
{
    vtkm::filter::contour::Slice sliceFilter;
    sliceFilter.SetImplicitFunction(m_implFuncControl.function());
    sliceFilter.SetMergeDuplicatePoints(false);
    sliceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);
    sliceFilter.SetActiveField(fieldName);

    output = sliceFilter.Execute(input);
}
