#include <vistle/module/module.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>
#include <vistle/core/normals.h>
#include <vistle/core/grid.h>
#include <vistle/core/database.h>
#include <vistle/core/unstr.h>
#include <vistle/core/coords.h>
#include <vistle/core/coordswradius.h>
#include <vistle/alg/objalg.h>

// FIXME: throw out unnecessary ghost cells

class Assemble: public vistle::Module {
    static const int NumPorts = 3;

public:
    Assemble(const std::string &name, int moduleID, mpi::communicator comm);
    ~Assemble();

    typedef std::array<const std::vector<vistle::Object::const_ptr> *, NumPorts> AssembleData;

private:
    bool prepare() override;
    bool compute() override;
    bool reduce(int t) override;
    bool assemble(const AssembleData &d);
    vistle::Port *m_in[NumPorts], *m_out[NumPorts];
    vistle::StringParameter *p_attribute = nullptr;
    std::string m_attribute;

    std::vector<std::vector<vistle::Object::const_ptr>>
        m_toCombine[NumPorts]; // a vector per timestep containing all objects to combine
    std::vector<std::map<std::string, std::vector<vistle::Object::const_ptr>>> m_toCombineAttribute
        [NumPorts]; // a vector per timestep containing a map of attribute values to all objects to combine
};

using namespace vistle;

Assemble::Assemble(const std::string &name, int moduleID, mpi::communicator comm): vistle::Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        m_in[i] = createInputPort("data_in" + std::to_string(i), "geometry with mapped data");
        m_out[i] = createOutputPort("data_out" + std::to_string(i), "geometry assembled into single block per rank");
    }

    p_attribute = addStringParameter("attribute", "attribute to match", "");

    setReducePolicy(message::ReducePolicy::PerTimestep);
}

Assemble::~Assemble()
{}

bool Assemble::prepare()
{
    for (int i = 0; i < NumPorts; ++i) {
        m_toCombine[i].clear();
        m_toCombineAttribute[i].clear();
    }
    m_attribute = p_attribute->getValue();

    return true;
}


bool Assemble::compute()
{
    Object::const_ptr oin[NumPorts];
    DataBase::const_ptr din[NumPorts];
    Object::const_ptr grid;
    Normals::const_ptr normals;
    std::vector<const Scalar *> floats;

    int tt = -1;
    bool haveAttr = m_attribute.empty();
    std::string attr;

    for (int i = 0; i < NumPorts; ++i) {
        if (!m_in[i]->isConnected())
            continue;

        oin[i] = expect<Object>(m_in[i]);
        auto split = splitContainerObject(oin[i]);
        din[i] = split.mapped;
        Object::const_ptr g = split.geometry;
        if (tt == -1) {
            tt = split.timestep;
        }
        if (!haveAttr && oin[i] && oin[i]->hasAttribute(m_attribute)) {
            haveAttr = true;
            attr = oin[i]->getAttribute(m_attribute);
        }
        if (grid) {
            if (g->getHandle() != grid->getHandle()) {
                sendError("Grids on input ports do not match");
                return true;
            }
        } else {
            auto gi = g->getInterface<GeometryInterface>();
            if (!gi) {
                sendError("Input does not have a grid on port %s", m_in[i]->getName().c_str());
                return true;
            }
            grid = g;
            if (!haveAttr && grid && grid->hasAttribute(m_attribute)) {
                haveAttr = true;
                attr = grid->getAttribute(m_attribute);
            }
            normals = split.normals;
            if (!haveAttr && normals && normals->hasAttribute(m_attribute)) {
                haveAttr = true;
                attr = normals->getAttribute(m_attribute);
            }
        }
    }

    assert(tt >= -1);
    unsigned t = tt + 1;

    for (int i = 0; i < NumPorts; ++i) {
        if (!m_in[i]->isConnected())
            continue;

        if (m_toCombine[i].size() <= t) {
            m_toCombine[i].resize(t + 1);
        }
        if (m_toCombineAttribute[i].size() <= t) {
            m_toCombineAttribute[i].resize(t + 1);
        }

        if (attr.empty()) {
            m_toCombine[i][t].push_back(oin[i]);
        } else {
            m_toCombineAttribute[i][t][attr].push_back(oin[i]);
        }
    }

    return true;
}

bool Assemble::reduce(int tt)
{
    std::cerr << "reduce(t=" << tt << ")" << std::endl;

    assert(tt >= -1);
    unsigned t = tt + 1;

    AssembleData comb;
    bool haveData = false;
    for (int i = 0; i < NumPorts; ++i) {
        if (m_toCombine[i].size() > t) {
            comb[i] = &m_toCombine[i][t];
            haveData = true;
        } else {
            comb[i] = nullptr;
        }
    }
    if (!assemble(comb)) {
        if (m_attribute.empty())
            sendError("assembly failed");
        else
            sendError("assembly of objects without attribute %s failed", m_attribute.c_str());
        return true;
    }

    const std::map<std::string, std::vector<Object::const_ptr>> *nonempty = nullptr;
    for (int i = 0; i < NumPorts; ++i) {
        if (m_toCombineAttribute[i].size() > t && !m_toCombineAttribute[i][t].empty()) {
            nonempty = &m_toCombineAttribute[i][t];
            break;
        }
    }

    if (!nonempty) {
        if (!haveData)
            sendError("no port with data");
        return true;
    }

    for (const auto &av: *nonempty) {
        const auto &attr = av.first;
        for (int i = 0; i < NumPorts; ++i) {
            if (m_toCombineAttribute[i].size() > t)
                comb[i] = &m_toCombineAttribute[i][t][attr];
            else
                comb[i] = nullptr;
        }
        if (!assemble(comb)) {
            sendError("assembly of objects without attribute %s of value %s failed", m_attribute.c_str(), attr.c_str());
            return true;
        }
    }

    for (int i = 0; i < NumPorts; ++i) {
        m_toCombine[i][t].clear();
        m_toCombineAttribute[i][t].clear();
    }

    return true;
}

bool Assemble::assemble(const Assemble::AssembleData &d)
{
    size_t numobj = 0;
    for (int i = 0; i < NumPorts; ++i) {
        if (!d[i])
            continue;

        if (d[i]->empty())
            continue;

        if (numobj == 0) {
            numobj = d[i]->size();
        } else {
            if (d[i]->size() != numobj) {
                sendError("object number mismatch on ports");
                return false;
            }
        }
    }

    Triangles::ptr ntri;
    Quads::ptr nquad;
    Indexed::ptr nidx;
    Coords::ptr ncoords;
    Object::ptr ogrid;
    Normals::ptr nnormals;
    DataBase::ptr dout[NumPorts];

    std::vector<Index> elOff(numobj + 1), clOff(numobj + 1), coordOff(numobj + 1), normOff(numobj + 1);
    std::vector<Index> dataOff[NumPorts];
    for (int i = 0; i < NumPorts; ++i) {
        dataOff[i].resize(numobj + 1);
    }

    bool flatGeometry = false;
    for (size_t n = 0; n < numobj; ++n) {
        Object::const_ptr oin[NumPorts];
        DataBase::const_ptr din[NumPorts];
        Object::const_ptr grid;
        Normals::const_ptr normals;
        std::vector<const Scalar *> floats;
        for (int i = 0; i < NumPorts; ++i) {
            if (!d[i])
                continue;
            oin[i] = d[i]->at(n);
            auto split = splitContainerObject(oin[i]);
            din[i] = split.mapped;
            Object::const_ptr g = split.geometry;
            if (grid) {
                if (g && g->getHandle() != grid->getHandle()) {
                    sendError("Grids on input ports do not match");
                    return true;
                }
            } else {
                if (!g) {
                    sendError("Input does not have a grid on port %s", m_in[i]->getName().c_str());
                    return true;
                }
                grid = g;
                normals = split.normals;
            }
        }

        if (auto tri = Triangles::as(grid)) {
            const Index *cl = tri->getNumCorners() > 0 ? tri->cl() : nullptr;
            if (!cl) {
                if (tri->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Triangle geometry on some but not all ports");
                    return true;
                }
            }
            clOff[n + 1] = clOff[n] + tri->getNumCorners();

            if (!ntri) {
                ntri.reset(new Triangles(0, 0));
                ogrid = ntri;
                ncoords = nidx;
            } else {
                assert(ogrid == ntri);
            }
        } else if (auto quad = Quads::as(grid)) {
            const Index *cl = quad->getNumCorners() > 0 ? quad->cl() : nullptr;
            if (!cl) {
                if (quad->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Triangle geometry on some but not all ports");
                    return true;
                }
            }
            clOff[n + 1] = clOff[n] + quad->getNumCorners();

            if (!nquad) {
                nquad.reset(new Quads(0, 0));
                ogrid = nquad;
                ncoords = nidx;
            } else {
                assert(ogrid == nquad);
            }
        } else if (auto idx = Indexed::as(grid)) {
            auto unstr = UnstructuredGrid::as(idx);
            const Index *cl = idx->getNumCorners() > 0 ? idx->cl() : nullptr;
            if (!cl) {
                if (idx->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Indexed geometry on some but not all ports");
                    return true;
                }
            }
            elOff[n + 1] = elOff[n] + idx->getNumElements();
            clOff[n + 1] = clOff[n] + idx->getNumCorners();

            if (!nidx) {
                nidx = idx->clone();
                nidx->resetArrays();
                nidx->resetCorners();
                nidx->resetElements();
                ogrid = nidx;
                ncoords = nidx;
            } else {
                assert(ogrid == nidx);
            }
        }

        if (auto coords = Coords::as(grid)) {
            coordOff[n + 1] = coordOff[n] + coords->getNumCoords();
            if (!ncoords) {
                assert(!ogrid);
                ncoords = coords->clone();
                ncoords->resetArrays();
                ogrid = ncoords;
            } else {
                assert(ogrid == ncoords);
            }
        }

        if (normals) {
            Index num = normals->getSize();
            normOff[n + 1] = normOff[n] + num;
            if (!nnormals) {
                nnormals = normals->clone();
                nnormals->resetArrays();
            }
        }
        for (int i = 0; i < NumPorts; ++i) {
            if (din[i]) {
                dataOff[i][n + 1] = dataOff[i][n] + din[i]->getSize();
                if (!dout[i]) {
                    dout[i] = din[i]->clone();
                    dout[i]->resetArrays();
                }
            }
        }
    }

    if (ntri) {
        ntri->cl().resize(clOff[numobj]);
        ntri->x().resize(coordOff[numobj]);
        ntri->y().resize(coordOff[numobj]);
        ntri->z().resize(coordOff[numobj]);
    }
    if (nidx) {
        nidx->cl().resize(clOff[numobj]);
        nidx->x().resize(coordOff[numobj]);
        nidx->y().resize(coordOff[numobj]);
        nidx->z().resize(coordOff[numobj]);
        nidx->el().resize(elOff[numobj] + 1);
        auto nunstr = UnstructuredGrid::as(ogrid);
        if (nunstr) {
            nunstr->tl().resize(elOff[numobj]);
        }
    }
    if (ncoords) {
        ncoords->x().resize(coordOff[numobj]);
        ncoords->y().resize(coordOff[numobj]);
        ncoords->z().resize(coordOff[numobj]);
        auto nrad = CoordsWithRadius::as(ogrid);
        if (nrad) {
            nrad->r().resize(coordOff[numobj]);
        }
    }
    if (nnormals) {
        nnormals->x().resize(normOff[numobj]);
        nnormals->y().resize(normOff[numobj]);
        nnormals->z().resize(normOff[numobj]);
        updateMeta(nnormals);
        if (ntri)
            ntri->setNormals(nnormals);
        if (nidx)
            nidx->setNormals(nnormals);
    }
    for (int i = 0; i < NumPorts; ++i) {
        if (dout[i])
            dout[i]->setSize(dataOff[i][numobj]);
    }

    for (size_t n = 0; n < numobj; ++n) {
        Object::const_ptr oin[NumPorts];
        DataBase::const_ptr din[NumPorts];
        Object::const_ptr grid;
        Normals::const_ptr normals;
        std::vector<const Scalar *> floats;
        for (int i = 0; i < NumPorts; ++i) {
            if (!d[i])
                continue;
            oin[i] = d[i]->at(n);
            auto split = splitContainerObject(oin[i]);
            din[i] = split.mapped;
            Object::const_ptr g = split.geometry;
            if (grid) {
                if (g && g->getHandle() != grid->getHandle()) {
                    sendError("Grids on input ports do not match");
                    return true;
                }
            } else {
                if (!g) {
                    sendError("Input does not have a grid on port %s", m_in[i]->getName().c_str());
                    return true;
                }
                grid = g;
                normals = split.normals;
            }
        }

        if (auto tri = Triangles::as(grid)) {
            const Index *cl = tri->getNumCorners() > 0 ? tri->cl() : nullptr;
            if (!cl) {
                if (tri->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Triangle geometry on some but not all ports");
                    return false;
                }
            }

            Index *ncl = ntri->cl().data();
            if (cl) {
                Index num = tri->getNumCorners();
                Index off = clOff[n];
                Index coff = coordOff[n];
                for (Index i = 0; i < num; ++i) {
                    ncl[off + i] = cl[i] + coff;
                }
            }
        } else if (auto quad = Quads::as(grid)) {
            const Index *cl = quad->getNumCorners() > 0 ? quad->cl() : nullptr;
            if (!cl) {
                if (quad->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Quad geometry on some but not all ports");
                    return false;
                }
            }

            Index *ncl = nquad->cl().data();
            if (cl) {
                Index num = quad->getNumCorners();
                Index off = clOff[n];
                Index coff = coordOff[n];
                for (Index i = 0; i < num; ++i) {
                    ncl[off + i] = cl[i] + coff;
                }
            }
        } else if (auto idx = Indexed::as(grid)) {
            auto unstr = UnstructuredGrid::as(idx);
            const Index *cl = idx->getNumCorners() > 0 ? idx->cl() : nullptr;
            if (!cl) {
                if (idx->getNumCoords() > 0)
                    flatGeometry = true;
            } else {
                if (flatGeometry) {
                    sendError("flat Indexed geometry on some but not all ports");
                    return false;
                }
            }

            Index *nel = nidx->el().data();
            Index *ncl = nidx->cl().data();

            if (cl) {
                Index num = idx->getNumCorners();
                Index off = clOff[n];
                Index coff = coordOff[n];
                for (Index i = 0; i < num; ++i) {
                    ncl[off + i] = cl[i] + coff;
                }
            }

            auto nunstr = UnstructuredGrid::as(ogrid);
            if (nunstr) {
                auto tl = &unstr->tl()[0];
                auto ntl = nunstr->tl().data();
                memcpy(ntl + elOff[n], tl, unstr->getNumElements() * sizeof(*tl));
            }

            {
                auto el = &idx->el()[0];
                Index off = elOff[n];
                Index coff = clOff[n];
                Index num = idx->getNumElements();
                for (Index e = 0; e < num; ++e) {
                    nel[off + e] = el[e] + coff;
                }
                nel[off + num] = el[num] + coff;
            }
        }

        if (auto coords = Coords::as(grid)) {
            Index off = coordOff[n];
            Index num = coords->getNumCoords();
            const Scalar *x = coords->x(), *y = coords->y(), *z = coords->z();
            auto &nx = ncoords->x(), &ny = ncoords->y(), &nz = ncoords->z();

            memcpy(&nx[off], x, num * sizeof(*x));
            memcpy(&ny[off], y, num * sizeof(*y));
            memcpy(&nz[off], z, num * sizeof(*z));
            auto nrad = CoordsWithRadius::as(ogrid);
            auto rad = CoordsWithRadius::as(grid);
            if (rad && nrad) {
                const Scalar *r = rad->r();
                auto &nr = nrad->r();
                memcpy(&nr[off], r, num * sizeof(*r));
            }
        }

        if (ogrid && n == 0) {
            ogrid->copyAttributes(grid);
        }
        updateMeta(ogrid);

        if (normals) {
            Index num = normals->getSize();

            const Scalar *x = normals->x(), *y = normals->y(), *z = normals->z();
            auto &nx = nnormals->x(), &ny = nnormals->y(), &nz = nnormals->z();
            Index off = normOff[n];
            memcpy(&nx[off], x, num * sizeof(*x));
            memcpy(&ny[off], y, num * sizeof(*y));
            memcpy(&nz[off], z, num * sizeof(*z));
        }

        for (int i = 0; i < NumPorts; ++i) {
            if (din[i]) {
                Index num = din[i]->getSize();
                Index off = dataOff[i][n];
                if (auto si = Vec<Scalar>::as(Object::const_ptr(din[i]))) {
                    auto so = Vec<Scalar>::as(Object::ptr(dout[i]));
                    assert(so);
                    const Scalar *x = si->x();
                    Scalar *ox = so->x().data();
                    memcpy(ox + off, x, num * sizeof(*x));
                } else if (auto vi = Vec<Scalar, 3>::as(Object::const_ptr(din[i]))) {
                    auto vo = Vec<Scalar, 3>::as(Object::ptr(dout[i]));
                    assert(vo);
                    const Scalar *x = vi->x(), *y = vi->y(), *z = vi->z();
                    Scalar *ox = vo->x().data(), *oy = vo->y().data(), *oz = vo->z().data();
                    memcpy(ox + off, x, num * sizeof(*x));
                    memcpy(oy + off, y, num * sizeof(*y));
                    memcpy(oz + off, z, num * sizeof(*z));
                }
            }
        }
    }

    for (int i = 0; i < NumPorts; ++i) {
        if (dout[i]) {
            dout[i]->setGrid(ogrid);
            updateMeta(dout[i]);
            addObject(m_out[i], dout[i]);
        } else if (d[i] && !d[i]->empty()) {
            addObject(m_out[i], ogrid);
        }
    }

    return true;
}


MODULE_MAIN(Assemble)
