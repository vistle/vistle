#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/coords.h>

#include "AttachNormals.h"

using namespace vistle;

MODULE_MAIN(AttachNormals)

AttachNormals::AttachNormals(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("grid_in", "grid or geometry");
    m_dataIn = createInputPort("data_in", "normals");

    m_gridOut = createOutputPort("grid_out", "grid with attached normals");
}

bool AttachNormals::compute()
{
    auto grid = expect<Coords>(m_gridIn);
    auto normals = expect<Normals>(m_dataIn);
    if (!grid || !normals)
        return true;

    auto out = grid->clone();
    out->setNormals(normals);
    updateMeta(out);
    addObject(m_gridOut, out);

    return true;
}
