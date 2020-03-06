#include "showGridIndex.h"

#include <core/unstr.h>

using namespace vistle;
ShowGridIndex::ShowGridIndex(const std::string& name, int moduleID, boost::mpi::communicator comm)
    :Module("Show the index of each vertex in the grid", name, moduleID, comm)
{

    m_gridIn = createInputPort("gridIn", "grid input");
    m_grid_out = createOutputPort("gridOut", "pass grid from grid in");
    m_indexOut = createOutputPort("indexOut", "the indicees of the vertices of the grid");

}

bool ShowGridIndex::compute() {

    Object::const_ptr input = accept<Object>(m_gridIn);
    auto unstr = UnstructuredGrid::as(input);
    if (!unstr) {
        return true;
    }
    Vec<Scalar, 1>::ptr indicees(new Vec<Scalar, 1>(unstr->getSize()));
    for (size_t i = 0; i < unstr->getSize(); ++i) {
        indicees->x().data()[i] = i;
    }
    indicees->setGrid(unstr);
    indicees->addAttribute("_species", "Indiceees");
    passThroughObject(m_grid_out, input);
    addObject(m_indexOut, indicees);


    return true;
}






MODULE_MAIN(ShowGridIndex)