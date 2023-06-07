#include <vistle/module/module.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>
#include <vistle/core/normals.h>
#include <vistle/core/grid.h>
#include <vistle/core/database.h>
#include <vistle/core/unstr.h>
#include <vistle/alg/objalg.h>

class WeldVertices: public vistle::Module {
    static const int NumPorts = 3;

public:
    WeldVertices(const std::string &name, int moduleID, mpi::communicator comm);
    ~WeldVertices();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    vistle::Port *m_in[NumPorts], *m_out[NumPorts];
};

using namespace vistle;

WeldVertices::WeldVertices(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        m_in[i] = createInputPort("data_in" + std::to_string(i), "input data");
        m_out[i] = createOutputPort("data_out" + std::to_string(i), "indexed data");
    }
}

WeldVertices::~WeldVertices()
{}

struct Point {
    Point(Scalar x, Scalar y, Scalar z, Index v, const std::vector<const Scalar *> &floats)
    : floats(floats), x(x), y(y), z(z), v(v)
    {}
    const std::vector<const Scalar *> &floats;
    Scalar x, y, z;
    Index v;

    bool operator<(const Point &o) const
    {
        if (x < o.x)
            return true;
        if (x > o.x)
            return false;

        if (y < o.y)
            return true;
        if (y > o.y)
            return false;

        if (z < o.z)
            return true;
        if (z > o.z)
            return false;

        for (auto f: floats) {
            if (f[v] < f[o.v])
                return true;
            if (f[v] > f[o.v])
                return false;
        }

        return false;
    }
};


bool WeldVertices::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr oin[NumPorts];
    DataBase::const_ptr din[NumPorts];
    Object::const_ptr grid;
    Normals::const_ptr normals;
    std::vector<const Scalar *> floats;

    for (int i = 0; i < NumPorts; ++i) {
        if (m_in[i]->isConnected()) {
            oin[i] = task->expect<Object>(m_in[i]);
            auto split = splitContainerObject(oin[i]);

            din[i] = split.mapped;
            Object::const_ptr g = split.geometry;
            if (din[i]) {
                if (din[i]->mapping() == DataBase::Vertex) {
                    if (auto s = Vec<Scalar, 1>::as(din[i])) {
                        floats.push_back(s->x());
                    } else if (auto v = Vec<Scalar, 3>::as(din[i])) {
                        floats.push_back(s->x());
                        floats.push_back(s->y());
                        floats.push_back(s->z());
                    }
                }
            }
            if (grid) {
                if (*split.geometry != *grid) {
                    sendError("Grids on input ports do not match");
                    return true;
                }
            } else {
                if (!split.geometry) {
                    sendError("Input does not have a grid on port %s", m_in[i]->getName().c_str());
                    return true;
                }
                grid = split.geometry;
                normals = split.normals;
                if (normals && normals->mapping() != DataBase::Element) {
                    floats.push_back(normals->x());
                    floats.push_back(normals->y());
                    floats.push_back(normals->z());
                }
            }
        }
    }

    auto coord = Coords::as(grid);
    if (!coord) {
        sendError("did not receive Coords as input");
        return true;
    }

    Object::ptr ogrid;
    std::vector<Index> remap;
    std::map<Point, Index> indexMap;
    if (auto tri = Triangles::as(grid)) {
        Index num = tri->getNumCorners();
        const Index *cl = num > 0 ? tri->cl() : nullptr;
        if (!cl)
            num = tri->getNumCoords();
        remap.reserve(num);
        const Scalar *x = tri->x(), *y = tri->y(), *z = tri->z();

        Triangles::ptr ntri(new Triangles(num, 0));
        Index *ncl = ntri->cl().data();

        if (cl) {
            Index count = 0;
            Index num = tri->getNumCorners();
            for (Index i = 0; i < num; ++i) {
                Index v = cl[i];
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[i] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        } else {
            Index count = 0;
            Index num = tri->getNumCoords();
            for (Index v = 0; v < num; ++v) {
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[v] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        }

        ogrid = ntri;
    } else if (auto quad = Quads::as(grid)) {
        Index num = quad->getNumCorners();
        const Index *cl = num > 0 ? quad->cl() : nullptr;
        if (!cl)
            num = quad->getNumCoords();
        remap.reserve(num);
        const Scalar *x = quad->x(), *y = quad->y(), *z = quad->z();

        Quads::ptr nquad(new Quads(num, 0));
        Index *ncl = nquad->cl().data();

        if (cl) {
            Index count = 0;
            Index num = quad->getNumCorners();
            for (Index i = 0; i < num; ++i) {
                Index v = cl[i];
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[i] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        } else {
            Index count = 0;
            Index num = quad->getNumCoords();
            for (Index v = 0; v < num; ++v) {
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[v] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        }

        ogrid = nquad;
    } else if (auto idx = Indexed::as(grid)) {
        Index num = idx->getNumCorners();
        const Index *cl = num > 0 ? idx->cl() : nullptr;
        if (!cl)
            num = idx->getNumCoords();
        const Scalar *x = idx->x(), *y = idx->y(), *z = idx->z();
        remap.reserve(num);

        Indexed::ptr nidx = idx->clone();
        nidx->resetArrays();
        nidx->resetCorners();
        nidx->cl().resize(num);

        Index *ncl = nidx->cl().data();

        if (cl) {
            Index count = 0;
            Index num = idx->getNumCorners();
            for (Index i = 0; i < num; ++i) {
                Index v = cl[i];
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[i] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        } else {
            Index count = 0;
            Index num = idx->getNumCoords();
            for (Index v = 0; v < num; ++v) {
                Point p(x[v], y[v], z[v], v, floats);
                auto &idx = indexMap[p];
                if (idx == 0) {
                    remap.push_back(v);
                    idx = ++count;
                }
                ncl[v] = idx - 1;
            }
            //sendInfo("found %d unique vertices among %d", count-1, num);
        }

        ogrid = nidx;
    }

    if (ogrid) {
        ogrid->copyAttributes(grid);
        updateMeta(ogrid);
    }
    auto ncoord = Coords::as(ogrid);
    if (!ncoord) {
        sendError("did not create Coords");
        return true;
    }

    const auto ix = &coord->x()[0];
    const auto iy = &coord->y()[0];
    const auto iz = &coord->z()[0];
    ncoord->setSize(remap.size());
    auto nx = &ncoord->x()[0];
    auto ny = &ncoord->y()[0];
    auto nz = &ncoord->z()[0];
    for (Index i = 0; i < remap.size(); ++i) {
        nx[i] = ix[remap[i]];
        ny[i] = iy[remap[i]];
        nz[i] = iz[remap[i]];
    }

    if (normals) {
        auto oc = Coords::as(ogrid);
        if (normals->mapping() == DataBase::Element) {
            oc->setNormals(normals);
        } else {
            auto nout = normals->clone();
            nout->resetArrays();
            nout->setSize(remap.size());
            const Scalar *x = normals->x(), *y = normals->y(), *z = normals->z();
            Scalar *ox = nout->x().data(), *oy = nout->y().data(), *oz = nout->z().data();
            for (Index i = 0; i < remap.size(); ++i) {
                ox[i] = x[remap[i]];
                oy[i] = y[remap[i]];
                oz[i] = z[remap[i]];
            }
            updateMeta(nout);
            oc->setNormals(nout);
        }
    }

    for (int i = 0; i < NumPorts; ++i) {
        if (din[i]) {
            if (din[i]->mapping() == DataBase::Element) {
                auto dout = din[i]->clone();
                dout->setGrid(ogrid);
                updateMeta(dout);
                task->addObject(m_out[i], dout);
            } else {
                auto dout = din[i]->clone();
                dout->resetArrays();
                dout->setSize(remap.size());
                if (auto si = Vec<Scalar>::as(Object::const_ptr(din[i]))) {
                    auto so = Vec<Scalar>::as(Object::ptr(dout));
                    assert(so);
                    const Scalar *x = si->x();
                    Scalar *ox = so->x().data();
                    for (Index i = 0; i < remap.size(); ++i) {
                        ox[i] = x[remap[i]];
                    }
                } else if (auto vi = Vec<Scalar, 3>::as(Object::const_ptr(din[i]))) {
                    auto vo = Vec<Scalar, 3>::as(Object::ptr(dout));
                    assert(vo);
                    const Scalar *x = vi->x(), *y = vi->y(), *z = vi->z();
                    Scalar *ox = vo->x().data(), *oy = vo->y().data(), *oz = vo->z().data();
                    for (Index i = 0; i < remap.size(); ++i) {
                        ox[i] = x[remap[i]];
                        oy[i] = y[remap[i]];
                        oz[i] = z[remap[i]];
                    }
                }
                dout->setGrid(ogrid);
                updateMeta(dout);
                task->addObject(m_out[i], dout);
            }
        } else {
            task->addObject(m_out[i], ogrid);
        }
    }

    return true;
}

MODULE_MAIN(WeldVertices)
