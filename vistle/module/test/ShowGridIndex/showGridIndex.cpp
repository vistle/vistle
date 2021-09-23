#include "showGridIndex.h"

#include <vistle/core/unstr.h>

using namespace vistle;
ShowGridIndex::ShowGridIndex(const std::string &name, int moduleID, boost::mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("gridIn", "grid input");
    m_grid_out = createOutputPort("gridOut", "pass grid from grid in");
    m_indexOut = createOutputPort("indexOut", "the indices of the vertices of the grid");
}

bool ShowGridIndex::compute()
{
    Object::const_ptr input = accept<Object>(m_gridIn);
    auto unstr = UnstructuredGrid::as(input);
    if (!unstr) {
        return true;
    }
    Vec<Scalar, 1>::ptr indices(new Vec<Scalar, 1>(unstr->getSize()));
    for (size_t i = 0; i < unstr->getSize(); ++i) {
        indices->x().data()[i] = i;
    }
    indices->setGrid(unstr);
    indices->addAttribute("_species", "Indices");
    addObject(m_indexOut, indices);

    auto ninput = input->clone();
    addObject(m_grid_out, ninput);

    return true;
}

MODULE_MAIN(ShowGridIndex)
