#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/lines.h>

#include "ShowCelltree.h"

MODULE_MAIN(ShowCelltree)

using namespace vistle;

enum ShowWhat {
    ShowLeft = 1,
    ShowRight = 2,
};

ShowCelltree::ShowCelltree(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    m_maxDepth = addIntParameter("maximum_depth", "maximum depth of nodes to show", 10);
    m_showLeft = addIntParameter("show_left", "show left separator", 1, Parameter::Boolean);
    m_showRight = addIntParameter("show_right", "show right separator", 1, Parameter::Boolean);

    createInputPort("grid_in");
    createOutputPort("grid_out");
    createOutputPort("data_out");
    createOutputPort("invalid_out");
}

ShowCelltree::~ShowCelltree()
{}

void visit(Celltree3::Node *nodes, Celltree3::Node &cur, Vector3 min, Vector3 max, Lines::ptr lines,
           Vec<Scalar>::ptr data, int depth, int maxdepth, int show)
{
    if (cur.isLeaf())
        return;

    if (depth > maxdepth)
        return;

    const int D = cur.dim;
    auto &el = lines->el();
    auto &x = lines->x();
    auto &y = lines->y();
    auto &z = lines->z();
    auto &cl = lines->cl();

    if (show & (ShowLeft | ShowRight)) {
        el.push_back(el.back() + 5);
        for (int i = 0; i < 5; ++i) {
            cl.push_back(x.size() + (i % 4));
        }
    }
    if ((show & ShowLeft) && (show & ShowRight)) {
        el.push_back(el.back() + 5);
        for (int i = 0; i < 5; ++i) {
            cl.push_back(x.size() + 4 + (i % 4));
        }
    }

    switch (D) {
    case 0:
        if (show & ShowLeft) {
            x.push_back(cur.Lmax);
            y.push_back(min[1]);
            z.push_back(min[2]);

            x.push_back(cur.Lmax);
            y.push_back(min[1]);
            z.push_back(max[2]);

            x.push_back(cur.Lmax);
            y.push_back(max[1]);
            z.push_back(max[2]);

            x.push_back(cur.Lmax);
            y.push_back(max[1]);
            z.push_back(min[2]);
        }

        if (show & ShowRight) {
            x.push_back(cur.Rmin);
            y.push_back(min[1]);
            z.push_back(min[2]);

            x.push_back(cur.Rmin);
            y.push_back(min[1]);
            z.push_back(max[2]);

            x.push_back(cur.Rmin);
            y.push_back(max[1]);
            z.push_back(max[2]);

            x.push_back(cur.Rmin);
            y.push_back(max[1]);
            z.push_back(min[2]);
        }
        break;

    case 1:
        if (show & ShowLeft) {
            x.push_back(min[0]);
            y.push_back(cur.Lmax);
            z.push_back(min[2]);

            x.push_back(min[0]);
            y.push_back(cur.Lmax);
            z.push_back(max[2]);

            x.push_back(max[0]);
            y.push_back(cur.Lmax);
            z.push_back(max[2]);

            x.push_back(max[0]);
            y.push_back(cur.Lmax);
            z.push_back(min[2]);
        }

        if (show & ShowRight) {
            x.push_back(min[0]);
            y.push_back(cur.Rmin);
            z.push_back(min[2]);

            x.push_back(min[0]);
            y.push_back(cur.Rmin);
            z.push_back(max[2]);

            x.push_back(max[0]);
            y.push_back(cur.Rmin);
            z.push_back(max[2]);

            x.push_back(max[0]);
            y.push_back(cur.Rmin);
            z.push_back(min[2]);
        }
        break;

    case 2:
        if (show & ShowLeft) {
            x.push_back(min[0]);
            y.push_back(min[1]);
            z.push_back(cur.Lmax);

            x.push_back(min[0]);
            y.push_back(max[1]);
            z.push_back(cur.Lmax);

            x.push_back(max[0]);
            y.push_back(max[1]);
            z.push_back(cur.Lmax);

            x.push_back(max[0]);
            y.push_back(min[1]);
            z.push_back(cur.Lmax);
        }

        if (show & ShowRight) {
            x.push_back(min[0]);
            y.push_back(min[1]);
            z.push_back(cur.Rmin);

            x.push_back(min[0]);
            y.push_back(max[1]);
            z.push_back(cur.Rmin);

            x.push_back(max[0]);
            y.push_back(max[1]);
            z.push_back(cur.Rmin);

            x.push_back(max[0]);
            y.push_back(min[1]);
            z.push_back(cur.Rmin);
        }
        break;
    }
    if (show & ShowLeft) {
        for (int i = 0; i < 4; ++i) {
            data->x().push_back(Scalar(depth));
        }
    }
    if (show & ShowRight) {
        for (int i = 0; i < 4; ++i) {
            data->x().push_back(Scalar(depth + 0.5));
        }
    }

    const Index L = cur.child;
    const Index R = cur.child + 1;
    Vector3 lmax = max;
    lmax[D] = cur.Lmax;
    Vector3 rmin = min;
    rmin[D] = cur.Rmin;
    ::visit(nodes, nodes[L], min, lmax, lines, data, depth + 1, maxdepth, show);
    ::visit(nodes, nodes[R], rmin, max, lines, data, depth + 1, maxdepth, show);
}

bool ShowCelltree::compute()
{
    auto obj = expect<Object>("grid_in");
    if (!obj) {
        sendError("no valid input data");
        return true;
    }

    Object::const_ptr gridObj;
    if (auto data = DataBase::as(obj)) {
        gridObj = data->grid();
    }
    if (!gridObj)
        gridObj = obj;

    auto cti = gridObj->getInterface<CelltreeInterface<3>>();
    if (!cti) {
        sendError("input does not support Celltree");
        return true;
    }

    bool celltreeValid = true;
    auto ct = cti->getCelltree();
    if (!cti->validateCelltree()) {
        sendError("Celltree verification failed on block %d, time=%d", gridObj->getBlock(), gridObj->getTimestep());
        celltreeValid = false;
    }

    vistle::Lines::ptr out(new vistle::Lines(Object::Initialized));
    vistle::Vec<Scalar>::ptr dataOut(new vistle::Vec<Scalar>(Object::Initialized));

    int show = 0;
    if (m_showLeft->getValue())
        show |= ShowLeft;
    if (m_showRight->getValue())
        show |= ShowRight;

    if (!ct->nodes().empty()) {
        Vector3 min(ct->min()[0], ct->min()[1], ct->min()[2]);
        Vector3 max(ct->max()[0], ct->max()[1], ct->max()[2]);
        ::visit(ct->nodes().data(), ct->nodes()[0], min, max, out, dataOut, 0, m_maxDepth->getValue(), show);
    }

    out->copyAttributes(gridObj);
    updateMeta(out);
    addObject("grid_out", out);
    dataOut->setGrid(out);
    updateMeta(dataOut);
    addObject("data_out", dataOut);
    if (celltreeValid) {
        vistle::Lines::ptr out(new vistle::Lines(Object::Initialized));
        out->copyAttributes(gridObj);
        vistle::Vec<Scalar>::ptr dataOut(new vistle::Vec<Scalar>(Object::Initialized));
        dataOut->setGrid(out);
        updateMeta(dataOut);
        addObject("invalid_out", dataOut);
    } else {
        addObject("invalid_out", dataOut);
    }

    return true;
}
