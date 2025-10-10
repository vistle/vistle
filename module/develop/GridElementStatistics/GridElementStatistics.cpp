#include "GridElementStatistics.h"
#include <vistle/core/unstr.h>
#include <vistle/core/vectortypes.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/alg/objalg.h>

using namespace vistle;

MODULE_MAIN(GridElementStatistics)

GridElementStatistics::GridElementStatistics(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "unstructured grid with or without data");
    createOutputPort("data_out", "unstructured grid with highlighted element");
    m_elementIndex = addIntParameter("element_index", "index of the element to inspect", 0);
}

bool GridElementStatistics::compute()
{
    auto obj = expect<Object>("grid_in");
    auto split = splitContainerObject(obj);
    if (!split.geometry) {
        sendError("no input geometry");
        return true;
    }

    auto grid = UnstructuredGrid::as(split.geometry);
    if (!grid) {
        sendError("cannot get underlying unstructured grid");
        return true;
    }

    Index elementIndex = m_elementIndex->getValue();
    if (elementIndex < 0) {
        sendError("invalid element index (negative)");
        return true;
    }
    if (elementIndex >= grid->getNumElements()) {
        sendError("invalid element index: %lu >= %lu", (unsigned long)elementIndex,
                  (unsigned long)grid->getNumElements());
        return true;
    }

    sendInfo("---");
    sendInfo("element index: %lu of block %d", (unsigned long)elementIndex, grid->getBlock());
    sendInfo("element type: %s", UnstructuredGrid::toString(UnstructuredGrid::Type(grid->tl()[elementIndex])));
    sendInfo("element dimensionality: %d", UnstructuredGrid::Dimensionality[grid->tl()[elementIndex]]);
    sendInfo("edge length: %f", grid->cellEdgeLength(elementIndex));
    sendInfo("surface: %f", grid->cellSurface(elementIndex));
    sendInfo("volume: %f", grid->cellVolume(elementIndex));

    auto highlight = std::make_shared<Vec<Scalar, 1>>(grid->getNumElements());
    std::fill(highlight->x().begin(), highlight->x().end(), 0);
    highlight->x()[elementIndex] = 1;
    highlight->setGrid(grid);

    highlight->setMapping(DataBase::Element);
    highlight->describe("highlight", id());
    updateMeta(highlight);
    addObject("data_out", highlight);

    return true;
}
