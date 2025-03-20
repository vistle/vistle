#include <vistle/alg/objalg.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/util/stopwatch.h>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>

#include "ClipVtkm.h"
#include <vistle/vtkm/convert.h>

using namespace vistle;

MODULE_MAIN(ClipVtkm)

ClipVtkm::ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3, false), m_implFuncControl(this)
{
    m_implFuncControl.init();
    m_flip = addIntParameter("flip", "flip inside out", false, Parameter::Boolean);
}

ClipVtkm::~ClipVtkm()
{}

bool ClipVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}

void ClipVtkm::runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                         vtkm::cont::DataSet &output) const
{
    vtkm::filter::contour::ClipWithImplicitFunction clipFilter;
    clipFilter.SetImplicitFunction(m_implFuncControl.function());
    clipFilter.SetInvertClip(m_flip->getValue() != 0);
    clipFilter.SetActiveField(fieldName);

    output = clipFilter.Execute(input);
}
