#include <vtkm/cont/DataSet.h>

#include <vtkm/filter/contour/Slice.h>

#include <vistle/alg/objalg.h>
#include <vistle/vtkm/convert.h>

#include "CuttingSurfaceVtkm.h"

MODULE_MAIN(CuttingSurfaceVtkm)

using namespace vistle;

CuttingSurfaceVtkm::CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm), m_implFuncControl(this)
{
    m_implFuncControl.init();
    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 0, Parameter::Boolean);
}

CuttingSurfaceVtkm::~CuttingSurfaceVtkm()
{}

bool CuttingSurfaceVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}

void CuttingSurfaceVtkm::runFilter(vtkm::cont::DataSet &filterInputData, vtkm::cont::DataSet &filterOutputData) const
{
    vtkm::filter::contour::Slice sliceFilter;
    sliceFilter.SetImplicitFunction(m_implFuncControl.func());
    sliceFilter.SetMergeDuplicatePoints(false);
    sliceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);
    sliceFilter.SetActiveField(m_mappedDataName);

    filterOutputData = sliceFilter.Execute(filterInputData);
}
