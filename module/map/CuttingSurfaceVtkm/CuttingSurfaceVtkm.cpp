#include <viskores/filter/contour/Slice.h>

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

void CuttingSurfaceVtkm::setInputSpecies(const std::string &species)
{
    setItemInfo(species);
}

std::unique_ptr<viskores::filter::Filter> CuttingSurfaceVtkm::setUpFilter() const
{
    auto filt = std::make_unique<viskores::filter::contour::Slice>();
    filt->SetImplicitFunction(m_implFuncControl.function());
    filt->SetMergeDuplicatePoints(false);
    filt->SetGenerateNormals(m_computeNormals->getValue() != 0);
    return filt;
}
