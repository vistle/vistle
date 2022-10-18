#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/coords.h>

#include "AttachGrid.h"

using namespace vistle;

MODULE_MAIN(AttachGrid)

AttachGrid::AttachGrid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("grid_in", "grid to attach");

    for (int i = 0; i < 5; ++i) {
        std::string number = std::to_string(i);
        m_dataIn.push_back(createInputPort("data_in" + number, "data"));
        m_dataOut.push_back(createOutputPort("data_out" + number, "data with input grid attached"));
    }
}

AttachGrid::~AttachGrid()
{}

bool AttachGrid::compute()
{
    bool ok = true;
    auto grid = expect<Object>(m_gridIn);
    Index numVert = 0;
    if (!grid) {
        ok = false;
    } else if (auto gi = grid->getInterface<GeometryInterface>()) {
        numVert = gi->getNumVertices();
    }

    std::vector<DataBase::const_ptr> data;
    for (size_t i = 0; i < m_dataIn.size(); ++i) {
        auto &pin = m_dataIn[i];
        if (isConnected(*pin)) {
            auto d = expect<DataBase>(pin);
            if (!d) {
                ok = false;
            }
            data.push_back(d);
        } else {
            data.push_back(DataBase::const_ptr());
        }
    }

    if (!ok) {
        std::cerr << "did not receive objects at all connected input ports" << std::endl;
        return true;
    }

    for (size_t i = 0; i < m_dataIn.size(); ++i) {
        auto &pin = m_dataIn[i];
        auto &pout = m_dataOut[i];
        if (isConnected(*pin)) {
            assert(data[i]);
            auto out = data[i]->clone();
            out->copyAttributes(data[i]);
            out->setGrid(grid);
            if (out->getSize() == numVert) {
                out->setMapping(DataBase::Vertex);
            } else {
                std::cerr << "cannot determine whether data is cell or vertex based: #vert grid=" << numVert
                          << ", #vert data=" << out->getSize() << std::endl;
            }
            updateMeta(out);
            addObject(pout, out);
        }
    }

    return true;
}
