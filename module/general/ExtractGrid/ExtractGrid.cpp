#include <vistle/core/object.h>
#include <vistle/core/coords.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/alg/objalg.h>
#include <vistle/module/resultcache.h>

#include "ExtractGrid.h"

using namespace vistle;

MODULE_MAIN(ExtractGrid)

ExtractGrid::ExtractGrid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_dataIn = createInputPort("data_in", "data mapped to geometry or grid");

    m_gridOut = createOutputPort("grid_out", "geometry or grid");
    m_normalsOut = createOutputPort("normals_out", "normals mapped to grid");

    addResultCache(m_gridCache);
    addResultCache(m_normalsCache);
}

bool ExtractGrid::compute()
{
    auto container = expect<Object>(m_dataIn);
    auto split = splitContainerObject(container);
    auto grid = split.geometry;
    auto normals = split.normals;
    Normals::ptr nnorm;

    if (normals && isConnected(*m_normalsOut)) {
        if (auto *c = m_normalsCache.getOrLock(normals->getName(), nnorm)) {
            nnorm = normals->clone();
            updateMeta(nnorm);
            m_normalsCache.storeAndUnlock(c, nnorm);
        }
        addObject(m_normalsOut, nnorm);
    }

    if (isConnected(*m_gridOut)) {
        Object::ptr ngrid;
        if (auto *c = m_gridCache.getOrLock(grid->getName(), ngrid)) {
            if (grid) {
                ngrid = grid->clone();
                if (auto coords = Coords::as(ngrid))
                    coords->setNormals(nnorm ? nnorm : normals);
                else if (auto str = StructuredGridBase::as(ngrid))
                    str->setNormals(nnorm ? nnorm : normals);
            }
            updateMeta(ngrid);
            m_gridCache.storeAndUnlock(c, ngrid);
        }
        addObject(m_gridOut, ngrid);
    }

    return true;
}
