#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>

#include "ScalarToVec.h"
#include <vistle/util/enum.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>

using namespace vistle;

ScalarToVec::ScalarToVec(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_vecOut = createOutputPort("data_out", "combined vector output");
    for (int i = 0; i < NumScalars; ++i) {
        m_scalarIn[i] = createInputPort("data_in" + std::to_string(i), "scalar input to be combined");
        linkPorts(m_scalarIn[i], m_vecOut);
    }
    m_species = addStringParameter("species", "Species for output", "");
}

bool ScalarToVec::compute()
{
    Vec<Scalar>::const_ptr data_in[NumScalars]{};
    int found = -1;
    Index size = InvalidIndex;
    Object::const_ptr grid;
    std::vector<std::string> names;
    for (int i = 0; i < NumScalars; ++i) {
        if (m_scalarIn[i]->isConnected()) {
            found = i;
            data_in[i] = expect<Vec<Scalar>>(m_scalarIn[i]);
            names.push_back(data_in[i]->getAttribute(attribute::Species));
            if (size == InvalidIndex) {
                size = data_in[i]->getSize();
            } else {
                if (size != data_in[i]->getSize()) {
                    std::cerr << "ScalarToVec: ERROR: Input sizes do not match" << std::endl;
                    return true;
                }
            }
            if (grid) {
                if (grid->getHandle() != data_in[i]->grid()->getHandle()) {
                    std::cerr << "ScalarToVec: ERROR: Grids do not match" << std::endl;
                    return true;
                }
            } else {
                grid = data_in[i]->grid();
            }
        }
    }

    if (found == -1) {
        std::cerr << "ScalarToVec: ERROR: No input data" << std::endl;
        return true;
    }
    assert(size != InvalidIndex);
    assert(!names.empty());

    std::string common = names[0];
    for (size_t i = 1; i < names.size(); ++i) {
        while (common.length() > 0) {
            if (names[i].substr(0, common.length()) == common) {
                break;
            }
            common.pop_back();
        }
    }

    std::string spec = m_species->getValue();
    if (spec.empty()) {
        spec = common.empty() ? boost::algorithm::join(names, "_") : common + "_vec";
    }

    Vec<Scalar, NumScalars>::ptr out(new Vec<Scalar, NumScalars>(size_t(0)));

    for (int i = NumScalars - 1; i >= 0; --i) {
        if (data_in[i]) {
            out->d()->x[i] = data_in[i]->d()->x[0];
            out->copyAttributes(data_in[i]);
        } else {
            out->d()->x[i]->resize(size, 0);
        }
    }
    out->setGrid(data_in[found]->grid());
    updateMeta(out);
    out->describe(spec, id());
    addObject(m_vecOut, out);

    return true;
}

MODULE_MAIN(ScalarToVec)
