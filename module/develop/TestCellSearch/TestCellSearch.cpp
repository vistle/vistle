#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/celltree.h>
#include <vistle/core/message.h>
#include "TestCellSearch.h"

MODULE_MAIN(TestCellSearch)

using namespace vistle;

TestCellSearch::TestCellSearch(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in", "grid");

    m_point = addVectorParameter("point", "search point", ParamVector(0., 0., 0.));
    m_block = addIntParameter("block", "number of containing block", -1);
    m_cell = addIntParameter("cell", "number of containing cell", -1);
    m_createCelltree = addIntParameter("create_celltree", "create celltree", 0, Parameter::Boolean);
}

TestCellSearch::~TestCellSearch()
{}

bool TestCellSearch::compute()
{
    const Vector3 point = m_point->getValue();

    auto gridObj = expect<Object>("data_in");
    if (!gridObj) {
        sendInfo("Did not receive valid object");
        return true;
    }
    auto grid = gridObj->getInterface<GridInterface>();
    if (!grid) {
        sendInfo("Unstructured or structured grid required");
        return true;
    }
    if (m_createCelltree->getValue()) {
        if (auto celltree = gridObj->getInterface<CelltreeInterface<3>>()) {
            if (!celltree->hasCelltree()) {
                celltree->getCelltree();
                if (!celltree->validateCelltree()) {
                    sendInfo("celltree validation failed for block %d", (int)gridObj->getBlock());
                }
            }
        }
    }
    Index idx = grid->findCell(point);
    if (idx != InvalidIndex) {
        setParameter(m_block, (Integer)gridObj->getBlock());
        setParameter(m_cell, (Integer)idx);

        const auto bounds = grid->cellBounds(idx);
        const auto &min = bounds.first, &max = bounds.second;

        std::cerr << "found cell " << idx << " (block " << gridObj->getBlock() << "): bounds: min " << min.transpose()
                  << ", max " << max.transpose() << std::endl;
    }

    return true;
}
