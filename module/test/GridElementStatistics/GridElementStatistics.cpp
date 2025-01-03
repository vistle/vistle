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
    m_elementIndex = addIntParameter("elementIndex", "index of the element to inspect", 0);
    setCacheMode(ObjectCache::CacheDeleteEarly);
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
    if (elementIndex < 0 || elementIndex >= grid->getNumElements()) {
        sendError("invalid element index");
        return true;
    }

    sendInfo("element index: %d of block %d", elementIndex, grid->getBlock());
    sendInfo("element type: %s", UnstructuredGrid::toString(UnstructuredGrid::Type(grid->tl()[elementIndex])));
    sendInfo("element dimensionality: %d", UnstructuredGrid::Dimensionality[grid->tl()[elementIndex]]);
    sendInfo("edge length: %f", grid->cellEdgeLength(elementIndex));
    sendInfo("surface: %f", grid->cellSurface(elementIndex));
    sendInfo("volume: %f", grid->cellVolume(elementIndex));
    sendInfo("");

    auto out = grid->clone();
    out->setMapping(DataBase::Vertex);
    auto highlight = make_ptr<Vec<Scalar, 1>>(grid->getNumVertices());
    std::fill(highlight->x().begin(), highlight->x().end(), 0);

    highlight->setGrid(out);
    for (size_t i = grid->el()[elementIndex]; i < grid->el()[elementIndex + 1]; i++) {
        highlight->x()[grid->cl()[i]] = 1;
    }


    highlight->setMapping(DataBase::Vertex);
    highlight->addAttribute("_species", "highlight");
    updateMeta(highlight);
    updateMeta(out);
    addObject("data_out", highlight);

    return true;
}
