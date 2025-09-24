
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                     (C)2005 Visenso ++
// ++ Description: Interpolation from Cell Data to Vertex Data            ++
// ++                 ( CellToVert module functionality  )                ++
// ++                                                                     ++
// ++ Author: Sven Kufer( sk@visenso.de)                                  ++
// ++                                                                     ++
// ++**********************************************************************/

#include "coCellToVert.h"
#include <vistle/core/vec.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>


namespace vistle {
inline double sqr(Scalar x)
{
    return double(x) * double(x);
}
} // namespace vistle

using namespace vistle;

////// workin' routines
template<typename S>
bool coCellToVert::interpolate(bool unstructured, Index num_elem, Index num_conn, Index num_point,
                               const Index *elem_list, const Index *conn_list, const Byte *type_list,
                               const Index *neighbour_cells, const Index *neighbour_idx, Index numComp, Index dataSize,
                               const S *in_data[], S *out_data[])
{
    // check for errors
    if (unstructured && !elem_list) {
        return false;
    }

    for (Index c = 0; c < numComp; ++c) {
        if (!in_data[c])
            return false;
        if (!out_data[c])
            return false;
    }

    // copy original data if already vertex based
    if (dataSize == num_point && dataSize != num_elem) {
        for (Index c = 0; c < numComp; ++c) {
            const S *in = in_data[c];
            S *out = out_data[c];
            for (Index i = 0; i < num_point; i++) {
                out[i] = in[i];
            }
        }
        return true;
    }

    return simpleAlgo(num_elem, num_conn, num_point, elem_list, conn_list, type_list, numComp, in_data, out_data);
}

template<typename S>
bool coCellToVert::simpleAlgo(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                              const Index *conn_list, const Byte *type_list, Index numComp, const S *in_data[],
                              S *out_data[])
{
    std::vector<Byte> weight(num_point);
    Byte *weight_num = weight.data();

    std::vector<Scalar> tmp;
    Scalar *tmp_out[3]{nullptr};

    tmp.resize(num_point * numComp);
    for (Index c = 0; c < numComp; ++c) {
        tmp_out[c] = tmp.data() + c * num_point;
    }

    if (elem_list && !conn_list && !type_list) {
        // structured grid
        for (Index i = 0; i < num_elem; i++) {
            auto verts = StructuredGridBase::cellVertices(i, elem_list);
            for (const auto &v: verts) {
                ++weight_num[v];
                for (Index c = 0; c < numComp; ++c) {
                    tmp_out[c][v] += in_data[c][i];
                }
            }
        }
    } else if (elem_list) {
        for (Index i = 0; i < num_elem; i++) {
            if (type_list && (type_list[i] == UnstructuredGrid::POLYHEDRON)) {
                Index begin = elem_list[i], end = elem_list[i + 1];
                std::vector<Index> verts;
                verts.reserve(end - begin);
                for (Index j = begin; j < end; ++j) {
                    verts.push_back(conn_list[j]);
                }
                std::sort(verts.begin(), verts.end());
                auto last = std::unique(verts.begin(), verts.end());
                for (auto it = verts.begin(); it != last; ++it) {
                    Index vertex = *it;
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        tmp_out[c][vertex] += in_data[c][i];
                    }
                }
            } else {
                const Index n = elem_list[i + 1] - elem_list[i];
                for (Index j = 0; j < n; j++) {
                    Index vertex = conn_list[elem_list[i] + j];
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        tmp_out[c][vertex] += in_data[c][i];
                    }
                }
            }
        }
    } else {
        // triangles/quads
        Index N = 0;
        if (conn_list)
            N = num_conn / num_elem;
        else
            N = num_point / num_elem;
        assert(N == 3 || N == 4);
        for (Index elem = 0; elem < num_elem; ++elem) {
            for (Index j = 0; j < N; ++j) {
                const Index vertex = conn_list ? conn_list[elem * N + j] : elem * N + j;
                ++weight_num[vertex];
                for (Index c = 0; c < numComp; ++c) {
                    tmp_out[c][vertex] += in_data[c][elem];
                }
            }
        }
        if (!conn_list) {
            // all weights are 1
            return true;
        }
    }

    // divide value sum by 'weight' (# adjacent cells)
    for (Index vertex = 0; vertex < num_point; vertex++) {
        if (weight_num[vertex] >= 1) {
            for (Index c = 0; c < numComp; ++c) {
                tmp_out[c][vertex] /= weight_num[vertex];
            }
        }
        for (Index c = 0; c < numComp; ++c) {
            out_data[c][vertex] = tmp_out[c][vertex];
        }
    }

    return true;
}

template<>
bool coCellToVert::simpleAlgo<Scalar>(Index num_elem, Index num_conn, Index num_point, const Index *elem_list,
                                      const Index *conn_list, const Byte *type_list, Index numComp,
                                      const Scalar *in_data[], Scalar *out_data[])
{
    // reset everything to 0, != 0 to prevent div/0 errors
    std::vector<unsigned char> weight(num_point);
    unsigned char *weight_num = weight.data();

    for (Index c = 0; c < numComp; ++c) {
        memset(out_data[c], 0, sizeof(Scalar) * num_point);
    }

    if (elem_list && !conn_list && !type_list) {
        // structured grid
        for (Index i = 0; i < num_elem; i++) {
            auto verts = StructuredGridBase::cellVertices(i, elem_list);
            for (const auto &v: verts) {
                ++weight_num[v];
                for (Index c = 0; c < numComp; ++c) {
                    out_data[c][v] += in_data[c][i];
                }
            }
        }
    } else if (elem_list) {
        // unstructured grid
        for (Index i = 0; i < num_elem; i++) {
            if (type_list && (type_list[i] == UnstructuredGrid::POLYHEDRON)) {
                Index begin = elem_list[i], end = elem_list[i + 1];
                std::vector<Index> verts;
                verts.reserve(end - begin);
                for (Index j = begin; j < end; ++j) {
                    verts.push_back(conn_list[j]);
                }
                std::sort(verts.begin(), verts.end());
                auto last = std::unique(verts.begin(), verts.end());
                for (auto it = verts.begin(); it != last; ++it) {
                    Index vertex = *it;
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        out_data[c][vertex] += in_data[c][i];
                    }
                }
            } else {
                const Index n = elem_list[i + 1] - elem_list[i];
                for (Index j = 0; j < n; j++) {
                    Index vertex = conn_list[elem_list[i] + j];
                    ++weight_num[vertex];
                    for (Index c = 0; c < numComp; ++c) {
                        out_data[c][vertex] += in_data[c][i];
                    }
                }
            }
        }
    } else {
        // triangles/quads
        Index N = 0;
        if (conn_list)
            N = num_conn / num_elem;
        else
            N = num_point / num_elem;
        assert(N == 3 || N == 4);
        for (Index elem = 0; elem < num_elem; ++elem) {
            for (Index j = 0; j < N; ++j) {
                const Index vertex = conn_list ? conn_list[elem * N + j] : elem * N + j;
                ++weight_num[vertex];
                for (Index c = 0; c < numComp; ++c) {
                    out_data[c][vertex] += in_data[c][elem];
                }
            }
        }
        if (!conn_list) {
            // all weights are 1
            return true;
        }
    }

    // divide value sum by 'weight' (# adjacent cells)
    for (Index vertex = 0; vertex < num_point; vertex++) {
        if (weight_num[vertex] >= 1) {
            for (Index c = 0; c < numComp; ++c) {
                out_data[c][vertex] /= (double)weight_num[vertex];
            }
        }
    }

    return true;
}


DataBase::ptr coCellToVert::interpolate(Object::const_ptr geo_in, DataBase::const_ptr data_in)
{
    if (!geo_in || !data_in) {
        return DataBase::ptr();
    }

    Index num_elem = 0, num_conn = 0, num_point = 0;
    const Index *elem_list = nullptr, *conn_list = nullptr;
    const Byte *type_list = nullptr;
    Index strSize[3] = {0, 0, 0};

    Index *neighbour_cells = nullptr;
    Index *neighbour_idx = nullptr;

    bool unstructured = false;
    if (auto sgrid_in = StructuredGridBase::as(geo_in)) {
        num_elem = sgrid_in->getNumElements();
        num_point = sgrid_in->getNumVertices();
        num_conn = 0;
        conn_list = nullptr;
        type_list = nullptr;
        for (int d = 0; d < 3; ++d)
            strSize[d] = sgrid_in->getNumDivisions(d);
        elem_list = strSize;
        unstructured = true;
    } else if (auto pgrid_in = Indexed::as(geo_in)) {
        num_elem = pgrid_in->getNumElements();
        num_point = pgrid_in->getNumCoords();
        num_conn = pgrid_in->getNumCorners();
        conn_list = pgrid_in->cl().data();
        elem_list = pgrid_in->el().data();
        if (auto ugrid_in = UnstructuredGrid::as(pgrid_in)) {
            unstructured = true;
            type_list = ugrid_in->tl().data();
        }
    } else if (auto tri = Triangles::as(geo_in)) {
        num_point = tri->getNumCoords();
        num_conn = tri->getNumCorners();
        num_elem = num_conn > 0 ? num_conn / 3 : num_point / 3;
        if (num_conn > 0)
            conn_list = tri->cl().data();
    } else if (auto qua = Quads::as(geo_in)) {
        num_point = qua->getNumCoords();
        num_conn = qua->getNumCorners();
        num_elem = num_conn > 0 ? num_conn / 4 : num_point / 4;
        if (num_conn > 0)
            conn_list = qua->cl().data();
    }

    const Index dataSize = data_in->getSize();
    if (dataSize < num_elem) {
        return DataBase::ptr();
    }

    Index numComp = 0;
    DataBase::ptr data_return;
    const Scalar *in_data[3] = {nullptr, nullptr, nullptr};
    Scalar *out_data[3] = {nullptr, nullptr, nullptr};
    const Index *in_data_i[3] = {nullptr, nullptr, nullptr};
    Index *out_data_i[3] = {nullptr, nullptr, nullptr};
    const Byte *in_data_b[3] = {nullptr, nullptr, nullptr};
    Byte *out_data_b[3] = {nullptr, nullptr, nullptr};

    if (auto tex_in = Texture1D::as(data_in)) {
        numComp = 1;
        in_data[0] = tex_in->x().data();

        Texture1D::ptr tex(new Texture1D(0, 0., 1.));
        tex->d()->pixels = tex_in->d()->pixels;
        tex->setSize(num_point);
        out_data[0] = tex->x().data();
        data_return = tex;
    } else if (auto s_data_in = Vec<Scalar>::as(data_in)) {
        numComp = 1;
        in_data[0] = s_data_in->x().data();

        Vec<Scalar>::ptr sdata(new Vec<Scalar>(num_point));
        out_data[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Scalar, 3>::as(data_in)) {
        numComp = 3;
        in_data[0] = v_data_in->x().data();
        in_data[1] = v_data_in->y().data();
        in_data[2] = v_data_in->z().data();

        Vec<Scalar, 3>::ptr vdata(new Vec<Scalar, 3>(num_point));
        out_data[0] = vdata->x().data();
        out_data[1] = vdata->y().data();
        out_data[2] = vdata->z().data();
        data_return = vdata;
    } else if (auto s_data_in = Vec<Index>::as(data_in)) {
        numComp = 1;
        in_data_i[0] = s_data_in->x().data();

        Vec<Index>::ptr sdata(new Vec<Index>(num_point));
        out_data_i[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Index, 3>::as(data_in)) {
        numComp = 3;
        in_data_i[0] = v_data_in->x().data();
        in_data_i[1] = v_data_in->y().data();
        in_data_i[2] = v_data_in->z().data();

        Vec<Index, 3>::ptr vdata(new Vec<Index, 3>(num_point));
        out_data_i[0] = vdata->x().data();
        out_data_i[1] = vdata->y().data();
        out_data_i[2] = vdata->z().data();
        data_return = vdata;
    } else if (auto s_data_in = Vec<Byte>::as(data_in)) {
        numComp = 1;
        in_data_b[0] = s_data_in->x().data();

        Vec<Byte>::ptr sdata(new Vec<Byte>(num_point));
        out_data_b[0] = sdata->x().data();
        data_return = sdata;
    } else if (auto v_data_in = Vec<Byte>::as(data_in)) {
        numComp = 3;
        in_data_b[0] = v_data_in->x().data();
        in_data_b[1] = v_data_in->y().data();
        in_data_b[2] = v_data_in->z().data();

        Vec<Byte, 3>::ptr vdata(new Vec<Byte, 3>(num_point));
        out_data_b[0] = vdata->x().data();
        out_data_b[1] = vdata->y().data();
        out_data_b[2] = vdata->z().data();
        data_return = vdata;
    }

    if (in_data[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, numComp, dataSize, in_data, out_data)) {
            return data_return;
        }
    } else if (in_data_i[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, numComp, dataSize, in_data_i, out_data_i)) {
            return data_return;
        }
    } else if (in_data_b[0]) {
        if (interpolate(unstructured, num_elem, num_conn, num_point, elem_list, conn_list, type_list, neighbour_cells,
                        neighbour_idx, numComp, dataSize, in_data_b, out_data_b)) {
            return data_return;
        }
    }

    return DataBase::ptr();
}
