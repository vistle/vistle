#include "showGridIndex.h"

#include <vistle/core/indexed.h>
#include <vistle/module/resultcache.h>
#include <vistle/alg/objalg.h>

using namespace vistle;
ShowGridIndex::ShowGridIndex(const std::string &name, int moduleID, boost::mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("gridIn", "grid input");
    m_indexOut = createOutputPort("indexOut", "the indices of the vertices of the grid");

    addResultCache(m_cache);
}

bool ShowGridIndex::compute()
{
    auto obj = accept<Object>(m_gridIn);
    auto split = splitContainerObject(obj);
    if (!split.geometry) {
        sendError("no grid on input data");
        return true;
    }
    auto input = Indexed::as(split.geometry);
    if (!input) {
        sendError("geometry is not indexed");
        return true;
    }
    Vec<Index, 1>::ptr indices;
    if (auto entry = m_cache.getOrLock(input->getName(), indices)) {
        indices.reset(new Vec<Index, 1>(input->getSize()));
        for (size_t i = 0; i < input->getSize(); ++i) {
            indices->x().data()[i] = i;
        }
        indices->setGrid(input);
        indices->describe("Indices", id());
        updateMeta(indices);
        m_cache.storeAndUnlock(entry, indices);
    }

    addObject(m_indexOut, indices);

    return true;
}

MODULE_MAIN(ShowGridIndex)
